# Getting Started with Coding

### Software
We will be primarly using [Visual Studio Code]([url](https://code.visualstudio.com/)) (VSC) and [GitHub]([url](https://desktop.github.com/download/)).

## Setting Up VSC
There are a few primary extensions you will want to add if you are working in C++ for the first time in VSC. Noteably, the C/C++ language from Microsoft
<img width="306" height="156" alt="Screenshot 2026-02-05 at 13 36 02" src="https://github.com/user-attachments/assets/ac8fb331-dd2a-4d85-a1d5-6f9d2b8c93f9" />
Also, look to install the serial monitor for later. 
<img width="1155" height="666" alt="Screenshot 2026-02-11 at 19 11 49" src="https://github.com/user-attachments/assets/1169bb0e-64f3-4185-a0fc-edc1f656ad70" />
You can also additionally add the Extension Pack and other similar C++ Intellisense options but it is not required. _If you have previously worked with C++ in your VSC enviornment please make sure to disable Clang when working on the Avionics code!_ There are a few issues with Clang and Platformio
<img width="728" height="692" alt="Screenshot 2026-02-05 at 13 38 08" src="https://github.com/user-attachments/assets/cb8a48ff-a3b7-4e95-afb8-6f3efbaa37da" />
Turning off or on an extension can simply be executed by the Enable/Disable button.
If you would like to test if your device is running C++ properly, try running this code
```
#include <iostream>
int main () {
    std::cout<<"Hello World!\n" << std::endl;
    return 0;
}
```
<img width="729" height="202" alt="Screenshot 2026-02-05 at 13 43 13" src="https://github.com/user-attachments/assets/c266bf5f-e821-45b6-ad46-de7b39c0ee83" />
You should see in your terminal the Hello World statement. **_If you cannot get this to work please reach out to me!_**

### Avionics Development
Instead of using ArudinoIDE, this is where PlatformIO comes in.
<img width="715" height="687" alt="Screenshot 2026-02-05 at 13 47 01" src="https://github.com/user-attachments/assets/0ca90dd2-7f98-4e6a-99e3-917f79464dce" />
This should be the extension that you are looking for, sometimes you may see on the left-hand side of your screen the little bug that is their mascot to quickly access the features of platformio. It is okay if this doesn't show up, as long as it is installed (which you can tell if it says untilstall and disable) then you are good to go.

You should then be taken to the Home page of PlatformIO, this is where we can make the compatibilty with the hardware and VSC.
<img width="1031" height="725" alt="Screenshot 2026-02-05 at 13 48 57" src="https://github.com/user-attachments/assets/85175073-22d2-4716-a6b4-ddbf0b48019c" />
You can add a new project yourself if you would like to try and create other projects on your own or if you want to start from scratch, but for now we will come back to this so we can link our Avionics code.

## GitHub
For many of you, GitHub is relativley new, just as a quick overall we are using GitHub like Loring said as more of a version control and it is a faster and more consise way to manage multiple versions and share data. There are three primary forms, Github.com (the website where you created your account), Git (the physical commands that mangage GitHub repositories) and GitHub Desktop (which is a UI user-friendly version of Git). We will be primarily working in the desktop app since Git can be more finkey and knowledge intensive. 

### GitHub and VSC
First things first, we actually need some code to work with. Open up your VSC and on the welcome screen you can press Ctrl + P (or command). type in '>git' and you should see an option for 'Git: Clone'
<img width="1078" height="576" alt="Screenshot 2026-02-05 at 13 55 40" src="https://github.com/user-attachments/assets/33fcbdf6-8cf9-4405-94f5-5eca2d5cc8bd" />
This should then prompt you to link your GitHub to your VSC and open up a browser tab. This is essential since otherwise you will not be able to make any new changes to the code. 
<img width="1510" height="754" alt="Screenshot 2026-02-05 at 13 59 07" src="https://github.com/user-attachments/assets/d056a2b6-2261-4b5b-83ea-e56a1a411343" />
For example, I am just using a Repo that I needed for Harty Darty last semester, but **UNDER THE CODE BRANCH**__ click **Code** and copy the URL link **_DO NOT just copy the URL at the top of your screen!_** You can then take the link and paste it into your bar at the top and click on it to clone the repository. 
<img width="539" height="381" alt="Screenshot 2026-02-05 at 14 00 23" src="https://github.com/user-attachments/assets/adec889c-0403-45dc-97f8-d802f8582468" />
You should then be prompted to choose a location on your computer to put the cloned files. Make sure you keep your code in a spot you can easily access. You can then open the repository and see all of the same elements that are the in the same repo! This is super useful for later too if you find any online repositories and want to try out their code on our hardware, just a lot faster than using Ctrl C Ctrl V for multiple files.

### GitHub Desktop
Going back to Github Desktop, ensure that you are signed in--if you are you should be able to add a new repository. If this is your first time using the desktop app, it may look a little different. 
<img width="978" height="665" alt="Screenshot 2026-02-05 at 14 03 17" src="https://github.com/user-attachments/assets/3411ee80-5c90-402d-92fd-bbf0158f0830" />
You should be able to add the exisitng repo (this repo so TwoStageAvionicsBay) and link up to this repository! If you have any issues with this part feel free to reach out. Otherwise, you may see now in your VSC the added Git section of the Timeline as such; you'll also notice it says your current Repo at the bottom as well
<img width="1072" height="783" alt="Screenshot 2026-02-05 at 14 08 24" src="https://github.com/user-attachments/assets/3c4c40fe-119b-4409-ad29-9030cd596df9" />

### PlatformIO
I will do a quick rundown, but for now I won't go too far into technicalities. If you cloned the repo properly and have PlatformIO installed you will see either the little bug or at the bottom of your screen a House icon to take you to the home page.
<img width="1078" height="192" alt="Screenshot 2026-02-05 at 14 09 26" src="https://github.com/user-attachments/assets/e5c5f312-cbc0-45be-a4aa-8b81b76b18ca" />
If you click on this, it should take you to that home page we were talking about earlier
<img width="1071" height="783" alt="Screenshot 2026-02-05 at 14 10 20" src="https://github.com/user-attachments/assets/92098429-ce25-4bbd-94e0-0de82c4fb953" />
If you open project, note your path may be a little different or hard to find at first but you should be opening the 'StageTwo' File folder, you can note if it is the correct one if you see a folder inside of it called 'src'. This is where the PlatformIO.ini file is which basically tells VSC and PlatformIO what kind of hardware we are going to be coding on. Additionally, you should see a check mark and right arrow which we will use when we are checking if code is ready to flash. For now, that is it! Note you will have to do this PlatformIO setup each time you are opening the file for the first time so keep this handy as well as your path!

