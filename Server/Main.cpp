#include <Windows.h>
#include <iostream>
#include <print>

#include <Utils.hpp>
#include <Hook.hpp>

#define SkipAircraft

void RequestExitHook()
{
    return;
}

int64 GetNetModeHook(void*)
{
    return 1;
}

bool KickPlayerHook(int64, int64, int64)
{
    return false;
}

void SendClientMoveAdjustments(UNetDriver* NetDriver)
{
    for (UNetConnection* Connection : NetDriver->ClientConnections)
    {
        if (Connection == nullptr || Connection->ViewTarget == nullptr)
        {
            continue;
        }

        static void (*SendClientAdjustment)(APlayerController*) = decltype(SendClientAdjustment)(InSDKUtils::GetImageBase() + 0x7D516F0);

        if (APlayerController* PC = Connection->PlayerController)
        {
            SendClientAdjustment(PC);
        }

        for (UNetConnection* ChildConnection : Connection->Children)
        {
            if (ChildConnection == nullptr)
            {
                continue;
            }

            if (APlayerController* PC = ChildConnection->PlayerController)
            {
                SendClientAdjustment(PC);
            }
        }
    }
}

void (*TickFlushOriginal)(UNetDriver* NetDriver, float DeltaSeconds);
void TickFlushHook(UNetDriver* NetDriver, float DeltaSeconds)
{
    UReplicationSystem* ReplicationSystem = *(UReplicationSystem**)(int64(NetDriver) + 0x720);
    if (NetDriver->ClientConnections.Num() > 0 && ReplicationSystem)
    {
        static void (*UpdateReplicationViews)(UNetDriver*) = decltype(UpdateReplicationViews)(InSDKUtils::GetImageBase() + 0x7C7057C);
        static void (*PreSendUpdate)(UReplicationSystem*, float) = decltype(PreSendUpdate)(InSDKUtils::GetImageBase() + 0x724A8B0);
        UpdateReplicationViews(NetDriver);
        SendClientMoveAdjustments(NetDriver);
        PreSendUpdate(ReplicationSystem, DeltaSeconds);
    }

    TickFlushOriginal(NetDriver, DeltaSeconds);
}

bool (*ReadyToStartMatchOriginal)(AFortGameModeBR* GameMode);
bool ReadyToStartMatchHook(AFortGameModeBR* GameMode)
{
    if (UFortKismetLibrary::GetNumActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass()) <= 0)
        return false;
        
    static bool Started = false;
    if (!Started)
    {
        Started = true;

        auto Playlist = UObject::FindObject<UFortPlaylistAthena>("FortPlaylistAthena Playlist_DefaultSolo.Playlist_DefaultSolo");
        auto GameState = (AFortGameStateBR*)GameMode->GameState;
        GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
        GameState->OnRep_CurrentPlaylistInfo();
        
#ifdef SkipAircraft
        GameState->GamePhase = EAthenaGamePhase::None;
#else
        GameState->GamePhase = EAthenaGamePhase::Warmup;
#endif
        GameState->OnRep_GamePhase(EAthenaGamePhase::Setup);

        bool (*InitHost)(AOnlineBeaconHost*) = decltype(InitHost)(InSDKUtils::GetImageBase() + 0x7F160B0);
        bool (*PauseBeaconRequests)(AOnlineBeaconHost*, bool) = decltype(PauseBeaconRequests)(InSDKUtils::GetImageBase() + 0xA0FCB68);
        bool (*InitListen)(UNetDriver*, UWorld*, FURL&, bool, FString&) = decltype(InitListen)(InSDKUtils::GetImageBase() + 0x7F16608);
        void (*SetWorld)(UNetDriver*, UWorld*) = decltype(SetWorld)(InSDKUtils::GetImageBase() + 0x145DF18);

        auto Beacon = Utils::SpawnActor<AFortOnlineBeaconHost>();
        Beacon->ListenPort = 7777;

        InitHost(Beacon);
        PauseBeaconRequests(Beacon, false);

        auto NetDriver = Beacon->NetDriver;
        auto World = NetDriver->World;
        World->NetDriver = NetDriver;
        *(bool*)(int64(NetDriver) + 0x729) = true; // bIsUsingIris
        NetDriver->NetDriverName = UKismetStringLibrary::Conv_StringToName(L"GameNetDriver");

        FURL Url = {};
        Url.Port = 7776;
        FString Error;

        InitListen(NetDriver, NetDriver->World, Url, false, Error);
        SetWorld(NetDriver, NetDriver->World);

        World->LevelCollections[0].NetDriver = NetDriver;
        World->LevelCollections[1].NetDriver = NetDriver;

        GameMode->bWorldIsReady = true;

        World->ServerStreamingLevelsVisibility = Utils::SpawnActor<AServerStreamingLevelsVisibility>();
    }

    return ReadyToStartMatchOriginal(GameMode);
}

APawn* SpawnDefaultPawnForHook(AFortGameModeBR* GameMode, AFortPlayerControllerAthena* PlayerController, AActor* StartSpot)
{
    auto translivesmatter = StartSpot->GetTransform();

#ifdef SkipAircraft
    translivesmatter.Translation = {-40850, 20660, 10000};
#endif

    auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;

    static auto AbilitySet = UObject::FindObject<UFortAbilitySet>("FortAbilitySet GAS_AthenaPlayer.GAS_AthenaPlayer");
    for (auto Ability : AbilitySet->GameplayAbilities)
    {
        PlayerState->AbilitySystemComponent->K2_GiveAbility(Ability, 1, 1);
    }
    static auto TacSprint = UObject::FindClassFast("GA_Athena_TacticalSprint_C");
    static auto DoorBash = UObject::FindClassFast("GA_Athena_Player_DoorBash_C");
    PlayerState->AbilitySystemComponent->K2_GiveAbility(TacSprint, 1, 1);
    PlayerState->AbilitySystemComponent->K2_GiveAbility(DoorBash, 1, 1); // TODO Doesn't work correctly :(

    auto Pawn = GameMode->SpawnDefaultPawnAtTransform(PlayerController, translivesmatter);
    Pawn->bCanBeDamaged = false;
    return Pawn;
}

void ServerAcknowledgePossessionHook(AFortPlayerControllerAthena* PlayerController, APawn* P)
{
    PlayerController->AcknowledgedPawn = P;
}

void ServerAttemptAircraftJumpHook(UFortControllerComponent_Aircraft* Component, FRotator& ClientRotation)
{
    auto PlayerController = (AFortPlayerControllerAthena*)Component->GetOwner();
    auto Pawn = (AFortPlayerPawnAthena*)Utils::GetGameMode()->SpawnDefaultPawnAtTransform(PlayerController, Component->CurrentAircraft->GetTransform());
    Pawn->bCanBeDamaged = false;
    PlayerController->Possess(Pawn);
    PlayerController->ClientSetRotation(ClientRotation, false);
}

void InternalServerTryActivateAbility(UAbilitySystemComponent* Component, FGameplayAbilitySpecHandle Handle, bool InputPressed, const FPredictionKey& PredictionKey, const FGameplayEventData* TriggerEventData)
{
    static FGameplayAbilitySpec* (*FindAbilitySpecFromHandle)(UAbilitySystemComponent* Component, FGameplayAbilitySpecHandle Handle)
        = decltype(FindAbilitySpecFromHandle)(InSDKUtils::GetImageBase() + 0x11AA2C8);

    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Component, Handle);
    if (!Spec)
    {
        Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
        return;
    }

    const UGameplayAbility* AbilityToActivate = Spec->Ability;

    if (!AbilityToActivate)
    {
        Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
        return;
    }

    if (AbilityToActivate->NetSecurityPolicy == EGameplayAbilityNetSecurityPolicy::ServerOnlyExecution ||
        AbilityToActivate->NetSecurityPolicy == EGameplayAbilityNetSecurityPolicy::ServerOnly)
    {
        Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
        return;
    }

    UGameplayAbility* InstancedAbility = nullptr;
    Spec->InputPressed = true;

    static bool (*InternalTryActivateAbility)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, FPredictionKey, UGameplayAbility**, void*, const FGameplayEventData*)
        = decltype(InternalTryActivateAbility)(InSDKUtils::GetImageBase() + 0x8715398);

    if (InternalTryActivateAbility(Component, Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
    {
    }
    else
    {
        Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
        Spec->InputPressed = false;
        Utils::MarkArrayDirty(&Component->ActivatableAbilities);
    }
}

DWORD MainThread(HMODULE Module)
{
    AllocConsole();
    FILE* Dummy;
    freopen_s(&Dummy, "CONOUT$", "w", stdout);
    freopen_s(&Dummy, "CONIN$", "r", stdin);

    MH_Initialize();
    Hook::Function(InSDKUtils::GetImageBase() + 0x193E6EC, RequestExitHook);

    FMemory::Init((void*)(InSDKUtils::GetImageBase() + 0x6A929B0));

    Utils::ExecuteConsoleCommand(L"net.AllowEncryption 0");
    *(bool*)(InSDKUtils::GetImageBase() + 0x101FEA60) = true; // net.Iris.UseIrisReplication
    *(bool*)(InSDKUtils::GetImageBase() + 0x1008A1B3) = false; // GIsClient
    *(bool*)(InSDKUtils::GetImageBase() + 0x102CBE2C) = true; // GIsServer
    UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);

    Hook::Function(InSDKUtils::GetImageBase() + 0xFB9210, TickFlushHook, &TickFlushOriginal);
    Hook::Function(InSDKUtils::GetImageBase() + 0xEE9400, GetNetModeHook);
    Hook::Function(InSDKUtils::GetImageBase() + 0xA0F2B54, KickPlayerHook);

    Hook::VTable<AFortGameModeBR>(1824 / 8, SpawnDefaultPawnForHook);
    Hook::VTable<AFortGameModeBR>(2320 / 8, ReadyToStartMatchHook, &ReadyToStartMatchOriginal);
    Hook::VTable<AFortPlayerControllerAthena>(2448 / 8, ServerAcknowledgePossessionHook);
    Hook::VTable<UFortControllerComponent_Aircraft>(1320 / 8, ServerAttemptAircraftJumpHook);
    Hook::VTable<UFortAbilitySystemComponentAthena>(2216 / 8, InternalServerTryActivateAbility);

    Utils::ExecuteConsoleCommand(L"log LogFortUIDirector None");

    Utils::ExecuteConsoleCommand(L"open Asteria_Terrain");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
        break;
    }

    return TRUE;
}
