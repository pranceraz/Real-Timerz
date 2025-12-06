| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 | Linux |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | -------- | -------- | -------- | ----- |


##
ESP32 Rhythm Game running pureley on a ESP 32 



## Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `idf.py -p PORT monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.


Contribution Tips-
git config --global core.autocrlf false
type this in before committing and pushing // hold on this for now unless you see many errors 
on the building of compile commands.json is missing (its fine to press the button jsut make sure nothing gets committed / pushed to git (add the file name to gitignore if its getting tedious))
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
running in containers is great and is a secure way while ensuring crossplatform compatability 

