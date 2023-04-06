# WindowsQOL
Tool for making windows ux a little less awful.

## Features:
<details>
  <summary>Hot Corner</summary>
  Opens taskview when cursor is in the top left corner of the screen.
</details>

<details>
  <summary>Improved taskview</summary>
  Hover your cursor over the edge of the screen to switch desktops.
</details>

<details>
  <summary>Prevent Windows from going out of screen</summary>
  It does the above ^<br>
  Some windows cannot be snapped to edges/corners.<br>
  (Such as games e.g. btd6)<br>
  This saves you 20 seconds when positioning these apps.
</details>

## Installation:
### Get the executable:
- [Compile it yourself](#Compile-it-yourself)
- Download the exe from [releases](https://github.com/SigmaSkid/WindowsQOL/releases).  

### Add it to autostart:
1. Open the Task Scheduler by pressing Windows key + R, typing "taskschd.msc" and hitting Enter.
2. In the Task Scheduler, click "Create Task" in the "Actions" pane on the right side.
3. In the "General" tab, give your task a name and description.
4. Check the box "Run with highest privileges".
5. In the "Triggers" tab, click "New" and select "At log on" under "Begin the task".
6. In the "Actions" tab, click "New" and select "Start a program".
7. Browse to the location of the executable file and select it.
8. Click "OK" to create the task.

## Compile it yourself
1. Clone the repository.
2. Open the project in Microsoft Visual Studio 2019 or newer.
3. Select the appropriate build configuration (e.g. Debug or Release, x64).
4. Build the project using Visual Studio's build command (e.g. Build > Build Solution).
5. The compiled executable will be located in the project's output directory (e.g. \~\WindowsQOL\x64\Release|Debug).
