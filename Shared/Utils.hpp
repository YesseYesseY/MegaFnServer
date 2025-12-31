#include <SDK/FortniteGame_classes.hpp>
using namespace SDK;

namespace Utils
{
    template <typename T>
    T* SpawnObject(UObject* Outer)
    {
        return (T*)UGameplayStatics::SpawnObject(T::StaticClass(), Outer);
    }

    template <typename T>
    T* SpawnActor(FVector Pos = {}, FRotator Rot = {}, FVector Scale = { 1, 1, 1 })
    {
        auto translivesmatter = UKismetMathLibrary::MakeTransform(Pos, Rot, Scale);
        auto ret = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), T::StaticClass(), translivesmatter, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, nullptr);

        if (ret)
            ret = UGameplayStatics::FinishSpawningActor(ret, translivesmatter);

        return (T*)ret;
    }

    void ExecuteConsoleCommand(const wchar_t* Cmd)
    {
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), Cmd, nullptr);
    }

    AFortGameModeBR* GetGameMode()
    {
        return (AFortGameModeBR*)UWorld::GetWorld()->AuthorityGameMode;
    }

    void MarkArrayDirty(void* Arr)
    {
        static void (*uwu)(void*) = decltype(uwu)(InSDKUtils::GetImageBase() + 0x1CD6FD8);
        uwu(Arr);
    }

    template <typename T>
    T* GetSoftPtr(TSoftObjectPtr<T> SoftPtr)
    {
        auto ret = SoftPtr.Get();

        if (!ret)
            ret = (T*)UKismetSystemLibrary::LoadAsset_Blocking((TSoftObjectPtr<UObject>)SoftPtr);

        return (T*)ret;
    }

    UClass* GetSoftPtr(TSoftClassPtr<UClass>& SoftPtr)
    {
        auto ret = SoftPtr.Get();

        if (!ret)
            ret = UKismetSystemLibrary::LoadClassAsset_Blocking(SoftPtr);

        return ret;
    }

    UFortAssetManager* GetAssetManager()
    {
        return (UFortAssetManager*)UEngine::GetEngine()->AssetManager;
    }
}
