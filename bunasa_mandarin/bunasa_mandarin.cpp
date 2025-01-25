#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <cstdlib>  // For system() call
#include <chrono>   // For time handling
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include "Headers/info.h"

bool showUI = false;  // Flag to track if UI should be shown
bool showLogs = false;  // Flag to track if log window should be shown
bool showNetworkMonitor = false;  // Flag to track if network monitor window should be shown
std::atomic<bool> isPinging(false);  // Flag to control ping operation
std::vector<std::string> websiteStatuses;  // Stores the status of websites and IPs
std::vector<std::string> logs;  // Stores logs of past week
std::vector<std::string> websites = {"google.com", "8.8.8.8", "github.com", "192.168.1.1"};  // Websites and IPs to ping

// Ping function to get the status of a website or IP
std::string PingWebsite(const std::string& website)
{
    std::string command = "ping -c 1 " + website + " > /dev/null 2>&1";  // Suppress output
    int result = system(command.c_str());
    if (result == 0)
        return website + ": Up";
    else
        return website + ": Down";
}

// Function to generate logs
void GenerateLog(const std::string& status)
{
    logs.push_back(status);
    if (logs.size() > 1000) {  // Limit logs to last 1000 entries (about a week of logs)
        logs.erase(logs.begin());  // Remove the oldest entry
    }
}

// Function to fetch network stats
std::string GetNetworkStats()
{
    std::string command = "ifstat -i wlp3s0 1 1";  // Fetch network stats for 1 second on eth0 interface (use appropriate interface)
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

    if (showLogs)  // Render log window if the flag is true
    {
        ImGui::Begin("Status Logs");

        for (const auto& log : logs)
        {
            ImGui::Text("%s", log.c_str());
        }

        ImGui::End();
    }

    if (showNetworkMonitor)  // Render network monitor window if the flag is true
    {
        ImGui::Begin("Network Monitor");

        // Fetch and display network stats
        std::string stats = GetNetworkStats();
        ImGui::Text("Network Stats:\n%s", stats.c_str());

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_P && action == GLFW_PRESS)  // Check if "P" is pressed
    {
        showUI = !showUI;  // Toggle the UI visibility
        if (showUI && !isPinging) {
            isPinging = true;
            // Start a thread to ping websites every 10 seconds
            std::thread([]() {
                while (isPinging) {
                    websiteStatuses.clear();
                    for (const auto& website : websites) {
                        std::string status = PingWebsite(website);
                        websiteStatuses.push_back(status);
                        GenerateLog(status);  // Generate log entry for each ping
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(10));  // Wait 10 seconds before next ping
                }
            }).detach();
        }
    }

    if (key == GLFW_KEY_L && action == GLFW_PRESS)  // Check if "L" is pressed
    {
        showLogs = !showLogs;  // Toggle the log window visibility
    }

    if (key == GLFW_KEY_I && action == GLFW_PRESS)  // Check if "I" is pressed
    {
        showNetworkMonitor = !showNetworkMonitor;  // Toggle the network monitor window visibility
    }
}

using namespace std;

int main()
{
    cout << "Bunasa: VERSION: " << VERSION << " CODENAME: " << CODENAME << endl;
    cout << "Author: Allexander Bergmans." << endl;
    cout << "Co-Author: Toon Schuermans." << endl;

    // Initialize GLFW
    if (!glfwInit())
        return -1;

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(800, 600, "Bunasa", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Set the key callback
    glfwSetKeyCallback(window, KeyCallback);

    // Setup ImGui
    SetupImGui(window);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Render ImGui (based on the flag)
        RenderImGui();

        // Swap buffers
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
