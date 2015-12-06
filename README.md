# UnrealEngine-HTML-PakDownloader
Unreal Engine - HTML5 Download Paks on the fly

So, I've been trying to understand how HTML packaging's "Download maps on the fly" works underneath, and make sure that it works for me. Currently this feature is marked as experimental. If enabled, it keeps created pak files in a folder near your binaries and only packages the minimum necessities into your data file. But it has a problem.

If you have multiple levels, and want them to be downloaded by this feature, it will not work. Currently on-the-fly only downloads and mounts your first map from the game. Rest is currently up to you.

I wanted this feature badly. As the current project required multiple huge levels with all different assets inside. Creating a single data file resolves in GB's of file. Only option was to keep the pak files (that were simply 20-100 mbs) and download them in background, mount them then use if loaded correctly. Also it would be possible to download them while the user was changing levels (by adding a between-levels map to make sure that everything gets downloaded, if not already).

If you check the engine source, you'll find the responsible class is "MapPakDownloader". Which does its job perfectly and is really easy to read and understand. I simply got what I needed from this code, created a blueprintable class for the simple purpose of a working download-on-the-fly feature. Remember that this code is actually a prettified, and ripped-what-i-needed version of the "MapPakDownloader".

This repository contains a first person shooter game with multiple levels + the source code of the PAK downloader. Game is created in version 4.10 of UE but c++ code works in 4.9 also.

What is what.
-------------

In the H file, you can see that I've three delegates for download "progress", "error" and "completed" events.

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPackageDownloader_OnDownloadComplete, FString, mapName);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPackageDownloader_OnDownloadError, FString, mapName);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPackageDownloader_OnDownloadProgress, FString, mapName, int32, progress);

A static string array to keep track of mounted maps (as I was not able to find something in the core to check mounted pak files).

    static TArray<FString> MountedMaps;

A static method to get a refence to a PackageDownloader

	UFUNCTION(BlueprintPure, Category = "Package Downloader")
		static UPackageDownloader* GetPackageDownloader(bool& IsValid);

Delegate methods, and a bunch of public methods, & private variables.

CPP file includes a few other things. Two delegates for passing methods as event callbacks, a class to start the download and call those event callbacks. Also the main downloader for making everything work together.

What does the code do?
----------------------

First there is the FEmscriptenPAKHttpFileRequest class. This class is responsible for calling the emscripten wget2 method to download the pak file, and make the calls to delegates on any occuring events. It simply returns itself to static event methods to be able to call them. You can duplicate this process for most of the emscripten methods, which is a magical place :)

    #ifdef EMSCRIPTEN
    		emscripten_async_wget2(
    			TCHAR_TO_ANSI(*(Url + FString(TEXT("?rand=")) + FGuid::NewGuid().ToString())),
    			TCHAR_TO_ANSI(*FileName),
    			TCHAR_TO_ANSI(TEXT("GET")),
    			TCHAR_TO_ANSI(TEXT("")),
    			this,
    			&FEmscriptenPAKHttpFileRequest::OnLoad,
    			&FEmscriptenPAKHttpFileRequest::OnError,
    			&FEmscriptenPAKHttpFileRequest::OnProgress
    			);
    #endif

I had to define compiler arguments where emscripten (and import for emscripten.h file) are needed. This was because Visual Studio was not able to find the file (source listed in project does not include emcc source folder) and is already available in compile time. You can add a search path for emcc but this is sufficient enough for me.

UPackageDownloader has just a few methods. One to set the map name, and another to start downloading. You can see that there are a few lambda methods in there. These are passed to FEmscriptenPAKHttpFileRequest as event callbacks. You can think lambda methods as collapsed modules in the blueprint area (not the same but can be imagined as similar things).

How about the blueprint side?
-----------------------------

The sample game is the original first person shooter template, coming with UE4. I just copied everything from the original map to 3 others. There is a blueprint actor, which works as a gate between maps. These gates are also responsible for downloading the required pak file.

![enter image description here](http://emrahgunduz.com/wp-content/uploads/github/a.jpg)

The gate is a box that contains a collision box to detect player's entrance. A door to keep player outside until the map is mounted. Name of the map that the door loads and the current progress of the download. Plus a light so that inside can be seen :)

![enter image description here](http://emrahgunduz.com/wp-content/uploads/github/b.jpg)

Gate variable "Travel to map" is public which can be set from editor in map. So it is really easy to drag-drop a working downloader in the game. A construction script was built to show the name.

![enter image description here](http://emrahgunduz.com/wp-content/uploads/github/c.jpg)

Event Graph has a few parts in it, that are commented.
First, we listen for a gate entrance with an overlap test for the player.

![enter image description here](http://emrahgunduz.com/wp-content/uploads/github/d.jpg)

Second part gets reference to a package downloader and sets the map to download. If the map is already downloaded, the gate door opens. If not download starts.

![enter image description here](http://emrahgunduz.com/wp-content/uploads/github/e.jpg)

We need to assign our callback events before the download.
If we were unable to download the map, we change the text on the gate.
Progress is printed on gate door, too.

![enter image description here](http://emrahgunduz.com/wp-content/uploads/github/f.jpg)

![enter image description here](http://emrahgunduz.com/wp-content/uploads/github/h.jpg)

![enter image description here](http://emrahgunduz.com/wp-content/uploads/github/i.jpg)

When the download completes, gate door opens.

![enter image description here](http://emrahgunduz.com/wp-content/uploads/github/j.jpg)

![enter image description here](http://emrahgunduz.com/wp-content/uploads/github/g.jpg)

That's all. Package the game and test it for yourself.
- Do not forget to check the "list of maps" for packaging. You must define your maps (project properties -> packaging).

![enter image description here](http://emrahgunduz.com/wp-content/uploads/github/k.jpg)

- Also, remember to enable the "download maps on the fly" (project properties -> html)

![enter image description here](http://emrahgunduz.com/wp-content/uploads/github/l.jpg)

Some warnings:
--------------

You can download a few maps at the same time. But be careful, as the downloads are async you may download a single map twice. I wrote a simple check for this occasion, if the same map gets downloaded twice, the second time it does not get mounted. Trying to mount the same map will crash the game in browser. You can also add a few lines of code to check the array if the current map is already downloaded in the `OnFileDownloadProgress` callback, cancel the download and raise an `OnPackageDownloadCompleted` event.

If the internet connection breaks while a download continues, currently it crashes the game. In the eyes of UE4, this is a fatal issue. Simply add another if-else statement in `window.onerror` definition in the html file, to get rid of this fatal issue, or your game will be removed from the scene and downloader will not issue an error (I left the html file as-is, you must change it if you need it).

Currently the code does not work with "Level Transition - Delta packs". You'll need to keep track of where user is and which map he/she is going to go. Merge those to something like "LevelOne_LevelTwo" and check if you can download that map. If you cannot, use the "LevelTwo" file instead. As I do not need transition deltas in my game I removed the code from my version. Check "MapPakDownloader" if you need deltas, and implement to the code.

You may need to right click the project file and click "Generate Visual Studio project files" as the repository does not contain the Intermediate folder to save space.

The code and blueprint can be improved vastly. I just wanted to show a way for understanding how downloading and mounting a package for HTML games can be accomplished.
