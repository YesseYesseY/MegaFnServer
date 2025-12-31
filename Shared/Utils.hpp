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
}
