//#################################
//# A Ram disk C++ Based Compiler #
//#################################


// Notage
// ^ Outputs it as "main"
// Your File structure should be:
// ^^ main script is "controller.cxx"
// ^^ header is "Header.hxx"
// ^^ any modules are in a "Modules" sub directory



// Libs / Defs
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <string>
#include <vector>
#include <expected>
#include <sstream>
#include <fstream>
#include <cmath>
#include <thread>
#include <chrono>
#include <semaphore>
#include <cstdlib>
#include <future>
#include <thread>
#define gibbe return



//###########################
// STRUCTS
//###########################

// Config for input args
struct Config {
    bool debug = false;
    bool clear = false;
    bool optimize = false;
    bool fclear = false;
    bool clearBD = false;
    bool clearRD = false;
    bool help = false;
};

// Holds info for each compilation
struct CompileResult {
    std::string file;
    double seconds;
    int exitCode;
};

// Zero fluffing idea :P
CompileResult CompileFile(
    const std::string Module,
    const std::string GppString,
    const std::filesystem::path& RamDir
);



//###########################
// DEFINES
//###########################
int main(int argc, char* argv[]);
std::vector<std::string> ModuleList(std::string);
std::expected<Config, std::string> parse_args(int argc, char* argv[]);
inline void Splitter();
inline void clear();
void sleep(const std::string& time);
std::string exec(const char* cmd);
CompileResult CompileFile(const std::string Module, const std::string GppString, const std::filesystem::path& RamDir);
long long getAvailableMemoryKB();
int CreateRamDisk(const std::filesystem::path &RamDir);
int CheckBuildDir();
std::vector<std::string> findFilesNeedingRebuild();
void buildprint(const std::string& msg);
std::vector<std::future<CompileResult>> startAsyncCompiles(const std::vector<std::string>& modules, const std::filesystem::path& RamDir, const std::string& GppString);
int LinkObjects(const std::filesystem::path& RamDir, const std::string& outputName, const std::string& GppString);
void clearRamDir(const std::filesystem::path& RamDir);
void clearbuildDir(const std::filesystem::path& BuildDir);
long long getFileSizeKB(const std::string& path);
long long getDirectorySizeKB(const std::string& path);


//###########################
// "MARCO'S"
//###########################
inline void print(const std::string& msg) {
    std::cout << msg << std::endl;
}

inline void vprint(const std::string& msg) {
    std::cout << msg << std::endl;
}

inline void Splitter() {
    std::cout << "" << std::endl;
    std::cout << "------------------------------------" << std::endl;
    std::cout << "" << std::endl;
}

inline void clear() {
    system("clear");
}

void sleep(const std::string& time) {
    std::string slumbies = "time " + time;
    system(slumbies.c_str());
}

void buildprint(const std::string& msg) {
    std::cout << "[BUILD] " << msg << '\n';
}


//###########################
// MAIN COMPILER SCRIPT
//###########################
int main(int argc, char* argv[]){

    clear();
    auto CompilerTimerStart = std::chrono::steady_clock::now();

// Configuration
    unsigned int cores = std::thread::hardware_concurrency();
    if (cores == 0) cores = 1; // fallback
    int jobs = cores * 2;
    std::string GppString;
    GppString += "g++ -std=c++23";
    long long availKB = getAvailableMemoryKB();
    const std::filesystem::path RamDir = "/dev/shm/CXXCompiler";

// Script argument passing
    auto result = parse_args(argc, argv);
    if (!result) {
        std::cerr << "Error: " << result.error() << '\n';
        return 1;
    }
    Config config = result.value();
    if (config.debug){       // Check for --debug & sets its addon flags
        GppString += " -O0 -Wall -Wextra -DNDEBUG";
    }
    if (config.optimize){    // Check for --optimise & sets its addon flags
        GppString += " -Ofast -march=native -funroll-loops -fomit-frame-pointer -pipe";
    }

// Header / First display segment
    //clear();
    Splitter();
    print("CXX Compiler script");

// HELP SECTION / Only displays with the --help arg
    if (config.help){
        print("");
        std::cout << "--help" << "      Displays this help section" << std::endl;
        std::cout << "--debug" << "     Sets debugging args" << std::endl;
        std::cout << "--optimize" << "  Sets optimization args" << std::endl;
        std::cout << "--clear" << "     Clears both the .build and ram directory" << std::endl;
        std::cout << "--fclear" << "    Clears both without compiling" << std::endl;
        std::cout << "--clearRD" << "   Clears the ram directory" << std::endl;
        std::cout << "--clearBD" << "   Clears the .build" << std::endl;
        Splitter();
        gibbe 1;
    }

// FCLEAR / Clears the .build and RamDir without actually compiling
    if(config.fclear){
        print("");
        print(" 1) Clear Ram Disk");
        print(" 2) Clear .build Dir");
        print(" 3) Clears both dirs");
        std::cout << "Select an option: ";
        int fclear_opt;
        std::cin >> fclear_opt; // Get user input
        switch(fclear_opt){
            case 1:
            clearRamDir(RamDir);
            break;
            case 2:
            clearbuildDir(std::filesystem::current_path() / ".build");
            break;
            case 3:
            clearRamDir(RamDir);
            clearbuildDir(std::filesystem::current_path() / ".build");
            break;
        }
        Splitter();
        gibbe 1;
    }

// Info / second display segment
    Splitter();
    if (availKB >= 0){
        if ((availKB / 1024.0) > 1000){std::cout << "Available memory: " << (std::round((availKB / 1024.0 / 1024.0)* 1000.0) / 1000.0) << " GB\n";}
        else{std::cout << "Available memory: " << availKB / 1024.0 << " MB\n";}
    }
    else{std::cout << "Failed to read memory info\n"; return 1;}

    if ((availKB / 1024.0) > 1024){ print("Allowing Ram Disk");}
    else {print("Not enough ram (1Gb min)"); gibbe 1;}
    std::cout << "Allowing for " << jobs << " Jobs" << std::endl;

// To Be Compiled / third display segment
    Splitter();
    print("Detecting Modules");
    std::vector<std::string> Modules = ModuleList("Modules");
    for (const auto& name : Modules) {std::cout << "  " << name << '\n';}

// Open Ram Disk / preb build space
    Splitter();
    auto RSyncStart = std::chrono::steady_clock::now();
    int CheckForBuildDir = CheckBuildDir();
    if(CheckForBuildDir == 1){print("ERROR IN BUILD DIR"); return -1;}
    print("Opening Ram Disk");
    CreateRamDisk(RamDir);
    std::filesystem::path src = std::filesystem::current_path();
    std::cout << "Pushing into >> " << RamDir << std::endl;
    std::string RSyncP1 = "rsync -a \"" + src.string() + "/Modules/\" \"" + RamDir.string() + "/Modules/\"";
    std::string RSyncP2 = "rsync -a \"" + src.string() + "/controller.cxx\" \"" + RamDir.string() + "/\"";
    std::string RSyncP3 = "rsync -a \"" + src.string() + "/Header.hxx\" \"" + RamDir.string() + "/\"";
    std::string RSyncP4 = "rsync -a \"" + src.string() + "/.build/\" \"" + RamDir.string() + "/.build/\"";
    int RSyncS1 = std::system(RSyncP1.c_str());
    int RSyncS2 = std::system(RSyncP2.c_str());
    int RSyncS3 = std::system(RSyncP3.c_str());
    int RSyncS4 = std::system(RSyncP4.c_str());

    auto RSyncEnd = std::chrono::steady_clock::now();
    double duration = std::chrono::duration_cast<std::chrono::duration<double>>(RSyncEnd - RSyncStart).count();
    std::cout << "Transfer took: " << ((std::round(duration*100.0))/100.0) << "s" << std::endl;


// Check for needed rebuilds / fourth display segment
    Splitter();
    print("Checking Build Dates");
    std::vector<std::string> rebuildList = findFilesNeedingRebuild();
    if (rebuildList.empty()) {std::cout << "All targets are up to date.\n";}
    else {
        std::cout << "Rebuild needed for:\n";
        for (const auto& path : rebuildList){std::cout << "  " << path << '\n';}
    }


// Start Compilers / fifth display segment
    Splitter();
    print("Starting Compiler");
    auto tasks = startAsyncCompiles(rebuildList, RamDir, GppString);
    bool allOk = true;
    double totalTime = 0.0;
    for (auto& t : tasks) {
        CompileResult res = t.get();
        totalTime += res.seconds;
        std::cout << "  " << res.file << " : " << ((std::round(res.seconds *1000))/1000) << "s" << (res.exitCode == 0 ? " [OK]" : " [FAIL]") << '\n';
        if (res.exitCode != 0)
            allOk = false;
    }
    std::cout << (allOk ? "Build succeeded.\n" : "Build failed.\n");

// Linker / Sixth display segmen
    Splitter();
    print("Starting Linker");
    auto LINKTimerStart = std::chrono::steady_clock::now();
    int linkCode = LinkObjects(RamDir, "main", GppString);
    if (linkCode != 0){print("ERROR IN LINKING"); return 1;}
    auto LINKTimerEnd = std::chrono::steady_clock::now();
    double LinlerTimerduration = std::chrono::duration_cast<std::chrono::duration<double>>(LINKTimerEnd - LINKTimerStart).count();
    std::cout << "Link Time: " << ((std::round(LinlerTimerduration*1000))/1000) << "s"<< std::endl;

// Push back / Seventh diplay segment
    Splitter();
    std::cout << "Pushing back << " << RamDir << std::endl;
    std::string RSyncF1 = "rsync -a \"" + RamDir.string() + "/.build/\" \"" + src.string() + "/.build/\"";
    std::string RSyncF2 = "rsync -a \"" + RamDir.string() + "/main\" \"" + src.string() + "/\"";
    int RSyncFS1 = std::system(RSyncF1.c_str());
    int RSyncFS2 = std::system(RSyncF2.c_str());
    print("Executable file \"main\" is now compiled");
    if (config.clear){print("Clearing Ram Dir & .build"); clearRamDir(RamDir); clearbuildDir(std::filesystem::current_path() / ".build");}
    if (config.clearRD){print("Clearing Ram Dir"); clearRamDir(RamDir);}
    if (config.clearBD){print("Clearing .build"); clearbuildDir(std::filesystem::current_path() / ".build");}

// Gather Info
    auto CompilerTimerEnd = std::chrono::steady_clock::now();
    double CompilerTimerduration = std::chrono::duration_cast<std::chrono::duration<double>>(CompilerTimerEnd - CompilerTimerStart).count();
    long long sizemainKB = getFileSizeKB("main");
    long long sizedirKB = getDirectorySizeKB(".build");

// Info Dump / Eigth display
    Splitter();
    print("Build Info: ");
    std::cout << "Main File Size: " << sizemainKB << "KB" << std::endl;
    std::cout << "Build dir Size: " << sizedirKB << "KB" << std::endl;
    std::cout << "Total Time: " << ((std::round(CompilerTimerduration*1000))/1000) << "s"<< std::endl;
    


// Final Display
    Splitter();
    gibbe 0;
}



//###########################
// FUNCTIONS
//###########################

// Reads input args
std::expected<Config, std::string> parse_args(int argc, char* argv[]){
    Config config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--debug")           {config.debug = true;} 
        else if (arg == "--clear")      {config.clear = true;}
        else if (arg == "--fclear")      {config.fclear = true;} 
        else if (arg == "--optimize")   {config.optimize = true;}
        else if (arg == "--clearBD")    {config.clearBD = true;}
        else if (arg == "--clearRD")    {config.clearRD = true;}
        else if (arg == "--help")       {config.help = true;}
        else {gibbe std::unexpected("Unknown argument: " + arg);}
    }

    gibbe config;
}


// Reads thru modules directory & puts them in a vector list
std::vector<std::string> ModuleList(std::string ModuleDirectory){
    std::vector<std::string> Modules;           // Establishes the vector
    int pathlength = ModuleDirectory.length();

    for (const auto &entry : std::filesystem::directory_iterator(ModuleDirectory)){ // Adds each output into the vector
        if (entry.is_regular_file() && entry.path().extension() == ".cxx")
            Modules.push_back(entry.path());
    }

    gibbe Modules; // Returns the vector containing modules
}


// Takes in a command in CMD.c_str() format and return its output
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;

    // Use function pointer type explicitly to avoid GCC attribute warning
    using pipe_ptr = std::unique_ptr<FILE, int(*)(FILE*)>;

    pipe_ptr pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    gibbe result;
}


// Headless Compiling
CompileResult CompileFile(const std::string Module, const std::string GppString, const std::filesystem::path& RamDir){

    // Starts the timer
    auto start = std::chrono::steady_clock::now();

    // Sets up the compiler
    std::string objName = std::filesystem::path(Module).stem().string() + ".o";
    std::filesystem::path objPath = RamDir / ".build" / objName;
    std::string command = GppString + " -c " + Module + " -o " + objPath.string();

    // Exec the compiler
    int code = std::system(command.c_str());

    // End timer
    auto end = std::chrono::steady_clock::now();
    double duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();

    // return its stats
    return {objName, duration, code};
}


// Parse for ram remaining
long long getAvailableMemoryKB() {
    std::ifstream meminfo("/proc/meminfo");
    std::string key;
    long long value;
    std::string unit;
    long long available = -1;

    while (meminfo >> key >> value >> unit) {
        if (key == "MemAvailable:") {
            available = value; // in KB
            break;
        }
    }

    gibbe available; // returns -1 if not found
}


// Create the ram Disk enviroment
int CreateRamDisk(const std::filesystem::path &RamDir){
    try {
        if (!std::filesystem::exists(RamDir)) {
            std::filesystem::create_directories(RamDir);
            std::cout << "Created directory: " << RamDir << '\n';
        } else {
            std::cout << "Directory already exists: " << RamDir << '\n';

        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}


// Check if the build dir needs to be created
int CheckBuildDir() {
    const std::filesystem::path buildDir = std::filesystem::current_path() / ".build";

    int Out;

    if (!std::filesystem::exists(buildDir)) {
        try {
            std::filesystem::create_directory(buildDir);
            //std::cout << "Created directory: " << buildDir << '\n';
            Out = 0;
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error with directory: " << e.what() << '\n';
            Out = 1;
        }
    } else {
        //std::cout << "Directory already exists: " << buildDir << '\n';
        Out = 0;
    }

    return Out;
}


// returns a vector list containg modules (or the controller) that need rebuilding
std::vector<std::string> findFilesNeedingRebuild() {
    std::vector<std::string> needsRebuild;
    const std::filesystem::path srcDir = "Modules";
    const std::filesystem::path buildDir = ".build";

// Checks the Modules Date & add to vector if they need to be built
    if (std::filesystem::exists(srcDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(srcDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".cxx") {
                const std::string baseName = entry.path().stem().string(); // e.g. "Audio"
                const std::filesystem::path objFile = buildDir / (baseName + ".o");

                bool rebuild = false;

                if (!std::filesystem::exists(objFile)) {
                    rebuild = true;
                } else {
                    auto srcTime = std::filesystem::last_write_time(entry.path());
                    auto objTime = std::filesystem::last_write_time(objFile);
                    if (srcTime > objTime)
                        rebuild = true;
                }

                if (rebuild)
                    needsRebuild.push_back(entry.path().string()); // Full relative path
            }
        }
    }

//Checks the controllers date
    const std::filesystem::path controllerSrc = "controller.cxx";
    const std::filesystem::path controllerObj = buildDir / "controller.o";

    // If Controller exsists
    if (std::filesystem::exists(controllerSrc)) {
        bool rebuild = false;

        // Check dates & If needed, rebuild
        if (!std::filesystem::exists(controllerObj)) {rebuild = true;} 
        else {
            auto srcTime = std::filesystem::last_write_time(controllerSrc);
            auto objTime = std::filesystem::last_write_time(controllerObj);
            if (srcTime > objTime) {rebuild = true;}
        }

        // Pushes the build request
        if (rebuild) {needsRebuild.push_back(controllerSrc.string());} 
        else {std::cerr << "Warning: controller.cxx not found.\n";}
    }
    
    // Returns the need build vector
    return needsRebuild;
}


// Async compile launcher (takes a vector list)
std::vector<std::future<CompileResult>> startAsyncCompiles(const std::vector<std::string>& modules, const std::filesystem::path& RamDir, const std::string& GppString){
    std::vector<std::future<CompileResult>> tasks;
    std::filesystem::path buildDir = RamDir / ".build";

    for (const std::string& module : modules) {
        tasks.push_back(std::async(std::launch::async, [module, GppString, RamDir]() {
            return CompileFile(module, GppString, RamDir);
        }));
    }

    return tasks;
}


// Linking the compiled files
int LinkObjects(const std::filesystem::path& RamDir, const std::string& outputName, const std::string& GppString){
    std::filesystem::path buildDir = RamDir / ".build";
    std::filesystem::path outputPath = RamDir / outputName;
    std::string command = GppString;

    // add all object files
    for (auto& entry : std::filesystem::directory_iterator(buildDir)) {
        if (entry.path().extension() == ".o")
            command += " \"" + entry.path().string() + "\"";
    }

    // specify output file
    command += " -o \"" + outputPath.string() + "\"";

    int code = std::system(command.c_str());

    if (code == 0)
        print("Linked successfully: " + outputPath.string());
    else
        print("Link failed!");

    return code;
}


// Clears out the ram directory
void clearRamDir(const std::filesystem::path& RamDir) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(RamDir)) {
            std::filesystem::remove_all(entry.path());
            std::filesystem::remove_all(RamDir);
        }

        std::cout << "Cleared RAM directory: " << RamDir << '\n';
    } 
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[ERROR] Failed to clear RAM directory: " << e.what() << '\n';
    }
}


// Clears out the local .builld directory
void clearbuildDir(const std::filesystem::path& BuildDir) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(BuildDir)) {
            std::filesystem::remove_all(entry.path());
        }

        std::cout << "Cleared .build directory: " << BuildDir << '\n';
    } 
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[ERROR] Failed to .build directory: " << e.what() << '\n';
    }
}


// Gets a files size
long long getFileSizeKB(const std::string& path) {
    try {
        auto bytes = std::filesystem::file_size(path);
        return static_cast<long long>(bytes / 1024); // convert bytes to KB
    } catch (const std::filesystem::filesystem_error&) {return -1;} // return -1 if file doesn't exist or error occurs
}


// Gets a directorys size
long long getDirectorySizeKB(const std::string& path) {
    namespace fs = std::filesystem;
    long long totalBytes = 0;

    try {
        // Use the new range-based directory iterator form (C++23-friendly)
        for (const auto& entry : fs::recursive_directory_iterator(path, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file()) {
                totalBytes += static_cast<long long>(entry.file_size());
            }
        }
    } catch (const fs::filesystem_error&) {
        return -1; // invalid path or permission error
    }

    return (totalBytes + 1023) / 1024; // convert bytes → KB (rounded up)
}
