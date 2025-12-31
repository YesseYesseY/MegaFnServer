namespace Inventory
{
    void GiveItem(AFortPlayerControllerAthena* PlayerController, UFortWorldItemDefinition* ItemDef, int32 Count = -1)
    {
        if (Count == -1)
        {
            auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
            Count = ItemDef->GetMaxStackSize(PlayerState->AbilitySystemComponent);
        }
        auto Item = (UFortWorldItem*)ItemDef->CreateTemporaryItemInstanceBP(Count, 1);
        PlayerController->WorldInventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
        PlayerController->WorldInventory->Inventory.ItemInstances.Add(Item);
    }

    void Update(AFortPlayerControllerAthena* PlayerController)
    {
        PlayerController->WorldInventory->HandleInventoryLocalUpdate();
        Utils::MarkArrayDirty(&PlayerController->WorldInventory->Inventory);
    }

    void ServerExecuteInventoryItem(AFortPlayerControllerAthena* PlayerController, const FGuid& ItemGuid)
    {
        if (PlayerController->IsInAircraft())
            return;

        auto Pawn = (AFortPlayerPawnAthena*)PlayerController->Pawn;

        for (auto Entry : PlayerController->WorldInventory->Inventory.ReplicatedEntries)
        {
            if (UKismetGuidLibrary::EqualEqual_GuidGuid(Entry.ItemGuid, ItemGuid))
            {
                Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry.ItemDefinition, ItemGuid, {}, false);
                break;
            }
        }
    }
}
