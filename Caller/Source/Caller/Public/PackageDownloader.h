#pragma once

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif
#include "Delegate.h"
#include "Object.h"
#include "PackageDownloader.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPackageDownloader_OnDownloadComplete, FString, mapName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPackageDownloader_OnDownloadError, FString, mapName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPackageDownloader_OnDownloadProgress, FString, mapName, int32, progress);

static TArray<FString> MountedMaps;

UCLASS(Blueprintable, BlueprintType)
class CALLER_API UPackageDownloader : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "Package Downloader")
		static UPackageDownloader* GetPackageDownloader(bool& IsValid);

	UPROPERTY(BlueprintAssignable, Category = "Package Downloader")
		FPackageDownloader_OnDownloadComplete OnPackageDownloadCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Package Downloader")
		FPackageDownloader_OnDownloadError OnPackageDownloadError;

	UPROPERTY(BlueprintAssignable, Category = "Package Downloader")
		FPackageDownloader_OnDownloadProgress OnPackageDownloadProgress;

	UFUNCTION(BlueprintCallable, Category = "Package Downloader")
		UPackageDownloader *SetMapPakName(FString mapName, bool &alreadyDownloaded);

	UFUNCTION(BlueprintCallable, Category = "Package Downloader")
		UPackageDownloader *DownloadMapPak();

private:

	FString OriginalName;
	FString MapToCache;
	FString HostName;
	FString PakLocation;

};
