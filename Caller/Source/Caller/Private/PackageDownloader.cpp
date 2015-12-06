// Fill out your copyright notice in the Description page of Project Settings.

#include "Caller.h"
#include "PackageDownloader.h"

DECLARE_DELEGATE_OneParam(FDelegateFString, FString)
DECLARE_DELEGATE_OneParam(FDelegateInt32, int32)

class FEmscriptenPAKHttpFileRequest
{
public:
	typedef int32 Handle;
private:

	FString FileName;
	FString Url;

	FDelegateFString OnLoadCallBack;
	FDelegateInt32 OnErrorCallBack;
	FDelegateInt32 OnProgressCallBack;

public:

	FEmscriptenPAKHttpFileRequest()
	{}

	static void OnLoad(uint32 Var, void* Arg, const ANSICHAR* FileName)
	{
		FEmscriptenPAKHttpFileRequest* Request = reinterpret_cast<FEmscriptenPAKHttpFileRequest*>(Arg);
		Request->OnLoadCallBack.ExecuteIfBound(FString(ANSI_TO_TCHAR(FileName)));
	}

	static void OnError(uint32 Var, void* Arg, int32 ErrorCode)
	{
		FEmscriptenPAKHttpFileRequest* Request = reinterpret_cast<FEmscriptenPAKHttpFileRequest*>(Arg);
		Request->OnErrorCallBack.ExecuteIfBound(ErrorCode);
	}

	static void OnProgress(uint32 Var, void* Arg, int32 Progress)
	{
		FEmscriptenPAKHttpFileRequest* Request = reinterpret_cast<FEmscriptenPAKHttpFileRequest*>(Arg);
		Request->OnProgressCallBack.ExecuteIfBound(Progress);
	}

	void Process()
	{
		UE_LOG(LogDownloader, Warning, TEXT("Starting Download for %s"), *FileName);

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
	}

	void SetFileName(const FString& InFileName) { FileName = InFileName; }
	FString GetFileName() { return FileName; }

	void SetUrl(const FString& InUrl) { Url = InUrl; }
	void SetOnLoadCallBack(const FDelegateFString& CallBack) { OnLoadCallBack = CallBack; }
	void SetOnErrorCallBack(const FDelegateInt32& CallBack) { OnErrorCallBack = CallBack; }
	void SetOnProgressCallBack(const FDelegateInt32& CallBack) { OnProgressCallBack = CallBack; }
};

UPackageDownloader* UPackageDownloader::GetPackageDownloader(bool& IsValid)
{
	IsValid = false;

	UPackageDownloader *downloader = NewObject<UPackageDownloader>();

	if (!downloader) {
		UE_LOG(LogDownloader, Error, TEXT("Package downloader not created"));
		return NULL;
	}
	if (!downloader->IsValidLowLevel()) {
		UE_LOG(LogDownloader, Error, TEXT("Package downloader not created -- NOT VALID"));
		return NULL;
	}

	IsValid = true;
	return downloader;
}

UPackageDownloader::UPackageDownloader(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Nothing to initialize in class
}

UPackageDownloader* UPackageDownloader::SetMapPakName(FString mapName, bool &alreadyDownloaded)
{
	alreadyDownloaded = MountedMaps.Contains(mapName);

#ifdef EMSCRIPTEN
	ANSICHAR *LocationString = (ANSICHAR*)EM_ASM_INT_V({
		var hoststring = "http://" + location.host;
		var buffer = Module._malloc(hoststring.length);
		Module.writeAsciiToMemory(hoststring, buffer);
		return buffer;
	});
#else
	ANSICHAR *LocationString = "localhost";
#endif

	OriginalName = FString(mapName);
	HostName = FString(ANSI_TO_TCHAR(LocationString));
	PakLocation = FString(FApp::GetGameName()) / FString(TEXT("Content")) / FString(TEXT("Paks"));
	MapToCache = FString(mapName + ".pak");

	UE_LOG(LogDownloader, Warning, TEXT("Will download %s from %s"), *MapToCache, *PakLocation);

	return this;
}

UPackageDownloader* UPackageDownloader::DownloadMapPak()
{
	FEmscriptenPAKHttpFileRequest* PakRequest = new FEmscriptenPAKHttpFileRequest;

	FDelegateFString OnFileDownloaded = FDelegateFString::CreateLambda([=](FString Name) {

		UE_LOG(LogDownloader, Warning, TEXT("%s download complete!"), *PakRequest->GetFileName());

		bool alreadyMounted = MountedMaps.Contains(OriginalName);
		if (!alreadyMounted) {
			UE_LOG(LogDownloader, Warning, TEXT("Mounting..."), *Name);
			FCoreDelegates::OnMountPak.Execute(Name, 0);
			UE_LOG(LogDownloader, Warning, TEXT("%s Mounted!"), *Name);
		}

		MountedMaps.Emplace(OriginalName);
		OnPackageDownloadCompleted.Broadcast(OriginalName);

		delete PakRequest;
	});

	FDelegateInt32 OnFileDownloadProgress = FDelegateInt32::CreateLambda([=](int32 Progress) {
		OnPackageDownloadProgress.Broadcast(OriginalName, Progress);
		UE_LOG(LogDownloader, Warning, TEXT(" %s %d%% downloaded"), *PakRequest->GetFileName(), Progress);
	});

	FDelegateInt32 OnFileDownloadError = FDelegateInt32::CreateLambda([=](int32) {
		UE_LOG(LogDownloader, Fatal, TEXT("Could not find any Map Paks, exiting"), *PakRequest->GetFileName());
		OnPackageDownloadError.Broadcast(OriginalName);
		delete PakRequest;
	});

	PakRequest->SetFileName(PakLocation / MapToCache);
	PakRequest->SetUrl(HostName / PakLocation / MapToCache);
	PakRequest->SetOnLoadCallBack(OnFileDownloaded);
	PakRequest->SetOnProgressCallBack(OnFileDownloadProgress);
	PakRequest->SetOnErrorCallBack(OnFileDownloadError);
	PakRequest->Process();

	return this;
}
