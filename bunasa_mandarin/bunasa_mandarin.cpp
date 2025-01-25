#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <cstdlib>
#include <chrono>
#include <fstream>  // For writing to a file
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <mutex>  // For mutex to protect shared resources

#include "Headers/info.h"

bool showUI = false;
bool showLogs = false;
bool showNetworkMonitor = false;
bool showHelp = false;
bool showSystemInfo = false;

std::atomic<bool> isPinging(false);
std::vector<std::string> websiteStatuses;
std::vector<std::string> logs;
std::vector<std::string> websites = {"google.com", "8.8.8.8", "github.com", "192.168.1.1"};
std::mutex statusMutex;  

std::string PingWebsite(const std::string& website)
{
    std::string command = "ping -c 1 " + website + " > /dev/null 2>&1";  // Suppress output
    int result = system(command.c_str());
    if (result == 0)
        return website + ": Up";
    else
        return website + ": Down";
}



void GenerateLog(const std::string& status)
{
    logs.push_back(status);
    if (logs.size() > 1000)
    {
        logs.erase(logs.begin());
    }
}

std::string GetNetworkStats()
{
    std::string command = "ifstat -i wlp3s0 1 1";
    char buffer[128];
    std::string result = "";
    FILE* fp = popen(command.c_str(), "r");
    if (fp == nullptr) {
        return "Failed to fetch network stats";
    }
    while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
        result += buffer;
    }
    fclose(fp);
    return result;
}

// Fetch system uptime
std::string FetchSystemUptime() {
    char buffer[128];
    std::string uptime = "Uptime: ";
    FILE* fp = popen("uptime -p", "r");
    if (fp == nullptr) {
        uptime += "Error fetching uptime";
    } else {
        while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
            uptime += buffer;
        }
        fclose(fp);
    }
    return uptime;
}

std::string batteryStatus;
std::string batteryPercentage;

// Fetch battery status
std::string FetchBatteryStatus() {
    char buffer[128];
    std::string batteryStatus = "Battery: ";
    
    // Fetch battery percentage
    FILE* fp = popen("upower -i /org/freedesktop/UPower/devices/battery_BAT0 | grep percentage", "r");
    if (fp == nullptr) {
        batteryStatus += "Error fetching battery percentage";
    } else {
        while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
            batteryStatus += buffer;  // Add battery percentage to the status string
        }
        fclose(fp);
    }
    
    // Fetch charging status
    std::string chargingStatus = "Charging: ";
    fp = popen("upower -i /org/freedesktop/UPower/devices/battery_BAT0 | grep state", "r");
    if (fp == nullptr) {
        chargingStatus += "Error fetching charging status";
    } else {
        while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
            chargingStatus += buffer;  // Add charging status to the string
        }
        fclose(fp);
    }

    // Combine battery percentage and charging status
    return batteryStatus + " | " + chargingStatus;
}

// Fetch disk usage
std::string FetchDiskUsage() {
    char buffer[128];
    std::string diskUsage = "Disk Usage: ";
    FILE* fp = popen("df -h /", "r");  // Use "df -h /" to directly get the root disk usage
    if (fp == nullptr) {
        diskUsage += "Error fetching disk usage";
    } else {
        // Skip the first line (header) and capture the second line
        fgets(buffer, sizeof(buffer), fp);  // Skip header
        if (fgets(buffer, sizeof(buffer), fp) != nullptr) {
            diskUsage += buffer;  // Add the second line (disk usage information)
        }
        fclose(fp);
    }
    return diskUsage;
}

std::string FetchRAMUsage() {
    char buffer[128];
    std::string ramUsage = "RAM Usage: ";
    FILE* fp = popen("free -h | grep Mem", "r");  // Fetch memory info
    if (fp == nullptr) {
        ramUsage += "Error fetching RAM usage";
    } else {
        while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
            ramUsage += buffer;  // Add memory usage information
        }
        fclose(fp);
    }
    return ramUsage;
}




// Export logs to a .txt file
void ExportLogsToFile(const std::string& filename)
{
    std::ofstream outFile(filename);
    if (outFile.is_open())
    {
        for (const auto& log : logs)
        {
            outFile << log << std::endl;
        }
        outFile.close();
    }
    else
    {
        std::cerr << "Failed to open file for writing." << std::endl;
    }
}

void SetupImGui(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

void RenderImGui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (showHelp)
    {
        ImGui::Begin("Help");

        ImGui::Text("Help Screen:");
        ImGui::Text("P - Toggle Ping UI");
        ImGui::Text("L - Toggle Logs Window");
        ImGui::Text("I - Toggle Network Monitor");
        ImGui::Text("H - Show/Hide this Help Screen");

        ImGui::End();
    }

    if (showUI)  // Render UI only if the flag is true
    {
        ImGui::Begin("Ping Websites");

        // Iterate over the websites and show status
        for (const auto& status : websiteStatuses)
        {
            if (status.find("Up") != std::string::npos)
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", status.c_str());  // Green for up
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", status.c_str());  // Red for down
            }
        }

        ImGui::End();
    }
    if (showLogs)
    {
        ImGui::Begin("Status Logs");

        // Export Button to export logs to a text file
        if (ImGui::Button("Export Logs"))
        {
            ExportLogsToFile("logs.txt");  // Export the logs to logs.txt
        }

        for (const auto& log : logs)
        {
            ImGui::Text("%s", log.c_str());
        }

        ImGui::End();
    }

    if (showNetworkMonitor)
    {
        ImGui::Begin("Network Monitor");

        std::string stats = GetNetworkStats();
        ImGui::Text("Network Stats:\n%s", stats.c_str());

        ImGui::End();
    }

    if (showSystemInfo)
    {
        ImGui::Begin("System Information");

        std::string uptime = FetchSystemUptime();
        std::string battery = FetchBatteryStatus();  // Fetch battery status with charging info
        std::string disk = FetchDiskUsage();
        std::string ram = FetchRAMUsage();  // Fetch RAM usage

        ImGui::Text("%s", uptime.c_str());
        ImGui::Text("%s", battery.c_str());  // Show battery and charging info
        ImGui::Text("%s", disk.c_str());
        ImGui::Text("%s", ram.c_str());  // Show RAM usage

        ImGui::End();
    }


    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        showUI = !showUI;
        if (showUI && !isPinging) {
            isPinging = true;
            std::thread([]() {
                while (isPinging) {
                    std::lock_guard<std::mutex> lock(statusMutex);  // Lock to ensure thread safety
                    websiteStatuses.clear();
                    for (const auto& website : websites) {
                        std::string status = PingWebsite(website);
                        websiteStatuses.push_back(status);
                        GenerateLog(status);
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                }
            }).detach();
        }
    }

    if (key == GLFW_KEY_L && action == GLFW_PRESS)
    {
        showLogs = !showLogs;
    }

    if (key == GLFW_KEY_I && action == GLFW_PRESS)
    {
        showNetworkMonitor = !showNetworkMonitor;
    }

    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        showHelp = !showHelp;
    }

    if (key == GLFW_KEY_S && action == GLFW_PRESS)
    {
        showSystemInfo = !showSystemInfo;
    }
}

int main()
{
    std::cout << "Bunasa: VERSION: " << VERSION << " CODENAME: " << CODENAME << std::endl;
    std::cout << "Author: Allexander Bergmans." << std::endl;
    std::cout << "Co-Author: Toon Schuermans." << std::endl;

    if (!glfwInit())
        return -1;

    GLFWwindow* window = glfwCreateWindow(800, 600, "Bunasa", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetKeyCallback(window, KeyCallback);

    SetupImGui(window);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT);

        RenderImGui();

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
