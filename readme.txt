
ScriptHookVdx12 – A DirectX 12 Script Hook for GTA V Enhanced

Author: [shifuguru]
GitHub: shifuguru/ScriptHookVdx12

📌 About the Project
ScriptHookVdx12 is a custom Script Hook replacement for Grand Theft Auto V Enhanced Edition (PC) that does not rely on ScriptHookV.dll.
This project provides a DirectX 12-compatible solution for injecting custom scripts and modding GTA V Enhanced with native function execution, vehicle spawning, and an in-game ImGui-based debug menu.

🎯 Features
✅ Script Hook Alternative: No dependency on ScriptHookV.dll
✅ DirectX 12 Integration: Uses D3D12 and DXGI for rendering
✅ ImGui Debug Menu: Overlay menu rendered in-game
✅ Native Function Execution: Calls GTA V's internal natives dynamically
✅ Pattern Scanning: Finds offsets dynamically (e.g., GetNativeHandler)
✅ Vehicle Spawning: Allows spawning vehicles via native function calls
✅ Console Debugging: Outputs logs to ScriptHookVdx12.log🎯 Features

🚀 Installation
Requirements
GTA V Enhanced (PC)
Windows 10/11
Visual Studio 2022
C++ SDK (DirectX 12)


1️⃣ Build Instructions
1. Clone the repository:
  git clone https://github.com/shifuguru/ScriptHookVdx12.git
2. Open Visual Studio and compile the project in x64 Release mode.
3. Inject the resulting .dll into GTA V Enhanced using an injector.


2️⃣ How to Use
Press F4 to open the in-game debug menu.
Use the menu to spawn vehicles or execute custom scripts.
Logs and errors will be saved to ScriptHookDX12V.log.


🛠️ Development
Project Structure


Dependencies
ImGui (for UI rendering)
DirectX 12 (D3D12)
DXGI (SwapChain Hooking)


💡 Future Plans
🔹 Implement more native function support
🔹 Enhance pattern scanning for better memory hook stability
🔹 Improve input handling for better menu navigation
🔹 Add support for more modding capabilities


📜 Disclaimer
This project is for educational purposes only.
The author does not encourage online cheating or game piracy. Use responsibly in single-player mode.


📬 Contact
For contributions or issues, open an issue on GitHub or reach out via GitHub Discussions.
