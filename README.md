| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 | Linux |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | -------- | -------- | -------- | ----- |

# Hello World Example

Starts a FreeRTOS task to print "Hello World".

(See the README.md file in the upper level 'examples' directory for more information about examples.)

## How to use example

Follow detailed instructions provided specifically for this example.

Select the instructions depending on Espressif chip installed on your development board:

- [ESP32 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started/index.html)
- [ESP32-S2 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/get-started/index.html)


## Example folder contents

The project **hello_world** contains one source file in C language [hello_world_main.c](main/hello_world_main.c). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt` files that provide set of directives and instructions describing the project's source files and targets (executable, library, or both).

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── pytest_hello_world.py      Python script used for automated testing
├── main
│   ├── CMakeLists.txt
│   └── hello_world_main.c
└── README.md                  This is the file you are currently reading
```

For more information on structure and contents of ESP-IDF projects, please refer to Section [Build System](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html) of the ESP-IDF Programming Guide.

## Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `idf.py -p PORT monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.

## Technical support and feedback

Please use the following feedback channels:

* For technical queries, go to the [esp32.com](https://esp32.com/) forum
* For a feature request or bug report, create a [GitHub issue](https://github.com/espressif/esp-idf/issues)

We will get back to you as soon as possible.
--------------------------------------------------
MINE-
git config --global core.autocrlf false
type this in before committing and pushing // hold on this for now unless you see many errors 
on the building of compile commands.json is missing (its fine to press the button jsut make sure nothing gets committed / pushed to git (add the file name to gitignore if its getting tedious))
---------------------------
git tutorial Arnav -
git fetch  once before working(you need to be connected to see latest changes )
git merge to add changes 
(git pull does a git fetch + git merge but is less safe)
git add to add files to be commited
git commit to confirm changes 
    -after a git commit youll enter a VIM environment there you want to press "I"
        this will put you in Insert mode and you can start entering you commit message
    - press esc and the type in ":wq"
        the : is command mode w is write and q means quit 
    - youre now ready to push
    - alt method is to git commit -m "commit message"
always git fetch before pushing ideally do it  when you start working  and then do a git merge (as long as you have no changes of your own)
do another git status and if it says youre 1 commit ahead locally and one commit behind from the main then do a git rebase 
    this will put you on top of the latest commit  

git push to push changes online
git status to chack whats going on
--------------------------------------
Set up tips-> Arnav 
if youre unable to build out of the gate 
Try ->
1)opening it in the contatiner (the option shows as soon as you select the folder in VSCode)
    this should solve all issues and you should be able to build and run within the docker container 
    im not sure if you can still gti commit/push  from the container but try it out otherwise jsut open the folder int the terminal and you should be able to push from there as the changes are synchronised through SSH (as far as I know)
2)Resetting the ESP-IDF build settings 
    make sure to never push any extra config / build files 
    reset the config files through the IDF wizard the same way you did when you setup ESP32 for vsCode in the beginning

I reccomend doing both and using the second method as a fallback 
running in containers is great and is a secure way whil eensuring crossplatform compatability 
