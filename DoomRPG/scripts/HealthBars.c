#include "RPG.h"

#include "HealthBars.h"
#include "Monsters.h"
#include "Utils.h"

int RPGMap HealthBarID;
fixed RPGMap HealthBarX;
fixed RPGMap HealthBarY;

bool RPGMap HealthBarClipEnabled;
fixed RPGMap HealthBarAlpha;
int RPGMap HealthBarClipX;
int RPGMap HealthBarClipY;
int RPGMap HealthBarClipHeight;
int RPGMap HealthBarClipWidth;

void DrawBarText(HUDBarInfo *Info, str Text)
{
    if (GetActivatorCVar("drpg_healthbars_world"))
    {
        int Width = 800;
        int Height = 600;
        fixed BaseViewHeight = (CompatMode == COMPAT_DRLA ? 48 : 41);
        fixed HeightAdd;
        fixed Dist;
        fixed DrawDist;
        fixed DrawScale;
        fixed Angle;
        fixed VAngle;
        fixed Pitch;
        fixed X;
        fixed Y;
        
        // Calculate X/Y and distance
        X = Info->X - GetActorX(0);
        Y = Info->Y - GetActorY(0);
        DrawDist = Distance3D(Info->X, Info->Y, Info->Z, GetActorX(0), GetActorY(0), GetActorZ(0));
        if (DrawDist > 1024.0)
            DrawDist = 1024.0;
        if (DrawDist < 128.0)
            DrawDist = 128.0;
        DrawScale = DrawDist / 256;
        // DrawScale *= (fixed)GetPlayerInfo(PlayerNumber(), PLAYERINFO_DESIREDFOV) / 90.0;
        SetHudSize(Width * DrawScale, Height * DrawScale, true);
        
        // Calculate angles
        VAngle = VectorAngle(X, Y);
        Angle = (VAngle - GetActorAngle(0));
        
        // Special height adjustments because most monsters heights are just flat out wrong
        if (Contains(Info->Actor, "Cacodemon"))
            HeightAdd = 16.0;
        else if (Contains(Info->Actor, "HellKnight") || Contains(Info->Actor, "BaronOfHell"))
            HeightAdd = 16.0;
        else if (Contains(Info->Actor, "PainElemental"))
            HeightAdd = 16.0;
        else if (Contains(Info->Actor, "Fatso") || Contains(Info->Actor, "Mancubus"))
            HeightAdd = 8.0;
        else if (Contains(Info->Actor, "Revenant"))
            HeightAdd = 40.0;
        else if (Contains(Info->Actor, "Archvile"))
            HeightAdd = 24.0;
        else if (Contains(Info->Actor, "Cyberdemon"))
            HeightAdd = 16.0;
        else if (Contains(Info->Actor, "SpiderMastermind"))
            HeightAdd = 24.0;
        
        if ((Angle < 0.2 || Angle > 0.8))
        {
            Pitch = VectorAngle(VectorLength(X, Y), Info->Z + Info->Height + 28.0 + HeightAdd - (GetActorZ(0) - BaseViewHeight + GetActorViewHeight(0)));
            
            X = (Width / 2 - ((Width / 2) * Sin(Angle))) * DrawScale;
            Y = (Height / 2 - ((Height / 2) * Sin(Pitch) * 3 / 2) - ((Height / 2) * Sin(GetActorPitch(0)) * 3 / 2)) * DrawScale;
            
            if (HealthBarClipEnabled)
                SetHudClipRect(X + HealthBarClipX, Y + HealthBarClipY, HealthBarClipWidth, HealthBarClipHeight);
            HudMessage("%S", Text);
            EndHudMessage(HUDMSG_FADEOUT | HUDMSG_ALPHA, HBAR_ID + HealthBarID++, "Untranslated", (int)X + HealthBarX, (int)Y + HealthBarY, 0.05, 0.1, HealthBarAlpha);
            SetHudClipRect(0, 0, 0, 0);
        }
        else
        {
            HudMessage("");
            EndHudMessage(HUDMSG_PLAIN, 0, "Untranslated", 0, 0, 0);
        }
    }
    else
    {
        fixed X = GetActivatorCVar("drpg_healthbars_x");
        fixed Y = GetActivatorCVar("drpg_healthbars_y");
        
        SetHudSize(GetActivatorCVar("drpg_healthbars_width"), GetActivatorCVar("drpg_healthbars_height"), false);
        if (HealthBarClipEnabled)
            SetHudClipRect(X + HealthBarClipX, Y + HealthBarClipY, HealthBarClipWidth, HealthBarClipHeight);
        HudMessage("%S", Text);
        EndHudMessage(HUDMSG_FADEOUT | HUDMSG_ALPHA, HBAR_ID + HealthBarID++, "Untranslated", X + HealthBarX, Y + HealthBarY, 0.05, 0.1, HealthBarAlpha);
        SetHudClipRect(0, 0, 0, 0);
    }
}

void DrawBarSprite(HUDBarInfo *Info, str Sprite)
{
    SetFont(Sprite);
    DrawBarText(Info, "A");
}

// Handles HUD Health Bars
NamedScript void HealthBars()
{
    int PlayerNum = PlayerNumber();
    bool HideBar, DrawStats;
    int ID;
    
    // Initialize HUD information struct
    auto HUDBarInfo Info =
    {
        0,                          // TID
        false, false,               // Is Player? / Is Friendly?
        "", "", "",                 // Actor/Name
        0, 0, 0, NULL,              // Level/Rank/Auras
        0, 0, 0, 0, 0, false, 0, 0, // Health/Armor/Shield
        0, 0, 0, 0, 0, 0, 0, 0,     // Stats
        0.0, 0.0, 0.0, 0.0,         // Position
    };
    
    Start:
    
    while (GetUserCVar(PlayerNum, "drpg_healthbars"))
    {
        SetActivatorToTargetExtended(Players(PlayerNum).TID);
        
        Delay(1);
        
        // Don't continue if you're in a menu
        if (Players(PlayerNum).InMenu || Players(PlayerNum).InShop || Players(PlayerNum).OutpostMenu > 0 || Players(PlayerNum).InMinigame)
        {
            SetActivator(Players(PlayerNum).TID);
            goto Start;
        }
        
        // Reset bar drawing globals
        HealthBarX = 0;
        HealthBarY = 0;
        HealthBarAlpha = 1.0;
        
        HideBar = false;
        DrawStats = true;
        
        //reset HUD Info struct
        Info.TID = 0;
        Info.IsPlayer = false; Info.Friendly = false;
        Info.Actor = ""; Info.NameColor = ""; Info.Name = "";
        Info.Level = 0; Info.Rank = 0; Info.Flags = 0;
        Info.Health = 0; Info.HealthMax = 0; Info.SpawnHealth = 0; Info.Armor = 0;
        Info.ArmorMax = 0; Info.ShieldActive = false; Info.Shield = 0; Info.ShieldMax = 0;
        Info.Strength = 0; Info.Defense = 0; Info.Vitality = 0; Info.Energy = 0;
        Info.Regeneration = 0; Info.Agility = 0; Info.Capacity = 0; Info.Luck = 0;
        Info.X = 0.0; Info.Y = 0.0; Info.Z = 0.0; Info.Height = 0.0;
        
        // Actor
        Info.Actor = GetActorClass(0);
        
        // Friendly
        if (GetActorProperty(0, APROP_Friendly))
            Info.Friendly = true;
        
        // Populate Stats
        if (ClassifyActor(0) & ACTOR_PLAYER)
        {
            ID = FindPlayerID(ActivatorTID());
            
            Info.TID = Players(ID).TID;
            
            Info.IsPlayer = true;
            
            Info.NameColor = "";
            Info.Name = StrParam("%tS", ID + 1);
            Info.Level = Players(ID).Level;
            Info.Rank = Players(ID).RankLevel;
            Info.Aura = Players(ID).Aura;
            
            Info.Health = Players(ID).ActualHealth;
            Info.HealthMax = Players(ID).HealthMax;
            Info.SpawnHealth = 100;
            Info.Armor = CheckInventory("Armor");
            Info.ArmorMax = GetArmorInfo(ARMORINFO_SAVEAMOUNT);
            Info.ShieldActive = Players(ID).Shield.Active;
            Info.Shield = Players(ID).Shield.Charge;
            Info.ShieldMax = Players(ID).Shield.Capacity;
            
            Info.Strength = Players(ID).Strength;
            Info.Defense = Players(ID).Defense;
            Info.Vitality = Players(ID).Vitality;
            Info.Energy = Players(ID).Energy;
            Info.Regeneration = Players(ID).Regeneration;
            Info.Agility = Players(ID).Agility;
            Info.Capacity = Players(ID).Capacity;
            Info.Luck = Players(ID).Luck;
            
            Info.X = GetActorX(0);
            Info.Y = GetActorY(0);
            Info.Z = GetActorZ(0);
            Info.Height = GetActorPropertyFixed(0, APROP_Height);
        }
        else
        {
            MonsterStatsPtr MonStats = &Monsters[GetMonsterID(0)];
            
            // Conditions to not do things
            if (!MonStats->HealthBar)
                goto Start;
            if (MonStats->Flags & MF_NOSTATS || GetCVar("drpg_monster_levels") == 0)
                DrawStats = false;
            
            Info.TID = MonStats->TID;
            
            Info.IsPlayer = false;
            
            Info.NameColor = DetermineBestStatColor(MonStats);
            Info.Name = GetActorPropertyString(0, APROP_NameTag);
            Info.Level = MonStats->Level;
            Info.Rank = MonStats->Threat;
            Info.Flags = MonStats->Flags;
            Info.Aura = MonStats->Aura;
            
            Info.Health = GetActorProperty(0, APROP_Health);
            Info.HealthMax = MonStats->HealthMax;
            Info.SpawnHealth = MonStats->SpawnHealth;
            Info.Armor = 0;
            Info.ArmorMax = 0;
            Info.ShieldActive = false;
            Info.Shield = 0;
            Info.ShieldMax = 0;
            
            Info.Strength = MonStats->Strength;
            Info.Defense = MonStats->Defense;
            Info.Vitality = MonStats->Vitality;
            Info.Energy = MonStats->Energy;
            Info.Regeneration = MonStats->Regeneration;
            Info.Agility = MonStats->Agility;
            Info.Capacity = MonStats->Capacity;
            Info.Luck = MonStats->Luck;
            
            Info.X = GetActorX(0);
            Info.Y = GetActorY(0);
            Info.Z = GetActorZ(0);
            Info.Height = GetActorPropertyFixed(0, APROP_Height);
        }
        
        // Valid target
        if (ActivatorTID() != Players(PlayerNum).TID && ClassifyActor(0) & ACTOR_ALIVE && !(ClassifyActor(0) & ACTOR_WORLD))
        {
            SetActivator(Players(PlayerNum).TID);
            
            // Reset the health bar drawing ID
            HealthBarID = 0;
            
            // Draw the bar's components
            DrawBarBase(&Info);
            DrawBarFill(&Info);
            if (DrawStats)
            {
                DrawBarEmblems(&Info);
                DrawBarStats(&Info);
            }
            
            // LegenDoom Legendary Skull
            if (CompatMode == COMPAT_LEGENDOOM && CheckActorInventory(Info.TID, "LDLegendaryMonsterToken"))
            {
                HealthBarX -= 192.0;
                HealthBarY -= 65.0 + (int)(Sin(Timer() / 64.0) * 4);
                
                DrawBarSprite(&Info, "SCULC0");
            }
        }
        
        // Terminate if the player is dead
        if (GetActorProperty(Players(PlayerNum).TID, APROP_Health) <= 0) return;
    }
    
    Delay(1);
    goto Start;
}

void DrawBarBase(HUDBarInfo *Info)
{
    // Name
    if (GetActivatorCVar("drpg_healthbars_names"))
    {
        SetFont("BIGFONT");
        DrawBarText(Info, StrParam("%S%S", Info->NameColor, Info->Name));
    }
    
    HealthBarY += 24.0;
    
    // Bar Cross
    HealthBarX -= 108.0;
    HealthBarY -= 1.0;
    if (Info->Friendly)
        DrawBarSprite(Info, "HPCrossF");
    else
        DrawBarSprite(Info, "HPCross");
    HealthBarX += 108.0;
    HealthBarY += 1.0;
    
    // Draw Aura Overlays
    int NumAuras = 0;
    int Auras[AURA_MAX];
    for (int i = 0; i < AURA_MAX; i++)
        if (Info->Aura.Type[i].Active)
            Auras[NumAuras++] = i;
    if (NumAuras >= AURA_MAX) // Shadow Aura
        DrawBarSprite(Info, "HPBarAS");
    else if (NumAuras > 0)
    {
        int Aura = (Timer() / 35) % NumAuras;
        DrawBarSprite(Info, StrParam("HPBarA%d", Auras[Aura] + 1));
    }
    
    // Bar
    if (Info->Flags & MF_BOSS)
        DrawBarSprite(Info, "HPBarB");
    else if (Info->Flags & MF_MEGABOSS)
    {
        if (Contains(Info->Name, "Pride"))
            DrawBarSprite(Info, "HPBarB1");
        else if (Contains(Info->Name, "Sloth"))
            DrawBarSprite(Info, "HPBarB7");
    }
    else if (Contains(Info->Actor, "Marine") || Contains(Info->Actor, "PowerSuit"))
        DrawBarSprite(Info, "HPBarM");
    else
        DrawBarSprite(Info, "HPBar");
    
    // HP/Max HP and Armor
    // TODO: Do this better? It's kinda shit atm, build up a string instead maybe?
    SetFont("SMALLFONT");
    if (Info->ShieldActive)
    {
        if (Info->IsPlayer && Info->Armor > 0)
            if (Info->ArmorMax >= 10000) // Indestructible Armor
                DrawBarText(Info, StrParam("\Cv%d/%d \Ca[Indestructible]", Info->Shield, Info->ShieldMax));
            else
                DrawBarText(Info, StrParam("\Cv%d/%d \Cd[%d/%d]", Info->Shield, Info->ShieldMax, Info->Armor, Info->ArmorMax));
        else
            DrawBarText(Info, StrParam("\Cv%d/%d", Info->Shield, Info->ShieldMax));
    }
    else
    {
        if (Info->IsPlayer && Info->Armor > 0)
            if (Info->ArmorMax >= 10000) // Indestructible Armor
                DrawBarText(Info, StrParam("\Cd%d/%d \Ca[Indestructible]", Info->Health, Info->HealthMax));
            else
                DrawBarText(Info, StrParam("\Cd%d/%d \Cd[%d/%d]", Info->Health, Info->HealthMax, Info->Armor, Info->ArmorMax));
        else
            DrawBarText(Info, StrParam("\Cd%d/%d", Info->Health, Info->HealthMax));
    }
}

void DrawBarEmblems(HUDBarInfo *Info)
{
    HealthBarY = 48.0;
    
    if (Info->IsPlayer)
    {
        for (int i = 0; i < Info->Rank; i++)
        {
            HealthBarX = -55.0 + (i * 7.0);
            DrawBarSprite(Info, "RankEmb");
        }
    }
    else
    {
        for (int i = 1; i <= Info->Rank; i++)
        {
            HealthBarX = 36.0 + (i * 7.0);
            
            if (i == 1)
                DrawBarSprite(Info, "RankEmb1");
            else if (i >= 2 && i <= 4)
                DrawBarSprite(Info, "RankEmb2");
            else if (i == 5 || i == 6)
                DrawBarSprite(Info, "RankEmb3");
            else if (i == 7 || i == 8)
                DrawBarSprite(Info, "RankEmb4");
            else if (i == 9)
                DrawBarSprite(Info, "RankEmb5");
            else if (i == 10)
                DrawBarSprite(Info, "RankEmb6");
        }
    }
}

void DrawBarFill(HUDBarInfo *Info)
{
    int HealthPercent = 0;
    
    // Divide-by-zero check
    if (Info->HealthMax <= 0)
        HealthPercent = 0;
    else if (Info->IsPlayer)
        HealthPercent = (Info->Health * 100) / 100;
    else
        HealthPercent = (Info->Health * 100) / Info->SpawnHealth;
    
    // Prevent bar Underflow/overflow
    if (HealthPercent < 0)
        HealthPercent = 0;
    if (HealthPercent > 1000)
        HealthPercent = 1000;
    
    // Position the X coordinate for fill drawing
    HealthBarX = -96.1;
    
    // Shield Fill
    if (Info->ShieldActive)
    {
        int ShieldPercent = (Info->Shield * 100) / Info->ShieldMax;
        HealthBarClipEnabled = true;
        HealthBarClipX = (int)HealthBarX;
        HealthBarClipY = (int)(HealthBarY - 6);
        HealthBarClipWidth = ShieldPercent * 203 / 100;
        HealthBarClipHeight = (int)(HealthBarY + 6);
        
        DrawBarSprite(Info, "FillShld");
    }
    
    // Health fills
    for (int i = 9; i >= 0; i--)
        if (HealthPercent > 100 * i)
        {
            HealthBarClipEnabled = true;
            HealthBarClipX = (int)HealthBarX;
            HealthBarClipY = (int)(HealthBarY - 6);
            HealthBarClipWidth = (HealthPercent - (100 * i)) * 203 / 100;
            HealthBarClipHeight = (int)(HealthBarY + 6);
            
            DrawBarSprite(Info, StrParam("Fill%d", i + 1));
        }
    
    // Reset Clipping Rectangle
    HealthBarClipEnabled = false;
    HealthBarClipX = 0;
    HealthBarClipY = 0;
    HealthBarClipWidth = 0;
    HealthBarClipHeight = 0;
}

void DrawBarStats(HUDBarInfo *Info)
{
    HealthBarX = -96.1;
    HealthBarY = 48.0;
    
    // Stat Amounts
    SetFont("BIGFONT");
    DrawBarText(Info, StrParam("\Cj%d", Info->Level));
    HealthBarY += 20.0;
    DrawBarText(Info, StrParam("\Cg%d", Info->Strength));
    HealthBarX += 56.0;
    DrawBarText(Info, StrParam("\Cd%d", Info->Defense));
    HealthBarX += 56.0;
    DrawBarText(Info, StrParam("\Ca%d", Info->Vitality));
    HealthBarX += 56.0;
    DrawBarText(Info, StrParam("\Cn%d", Info->Energy));
    HealthBarX = -96.1;
    HealthBarY += 20.0;
    DrawBarText(Info, StrParam("\Ct%d", Info->Regeneration));
    HealthBarX += 56.0;
    DrawBarText(Info, StrParam("\Ci%d", Info->Agility));
    HealthBarX += 56.0;
    DrawBarText(Info, StrParam("\Ch%d", Info->Capacity));
    HealthBarX += 56.0;
    DrawBarText(Info, StrParam("\Cf%d", Info->Luck));
    
    HealthBarX = -107.0;
    HealthBarY = 48.0;
    
    // Stat Icons
    DrawBarSprite(Info, "MonLevel");
    HealthBarX = -96.0;
    HealthBarY += 40.0;
    DrawBarSprite(Info, "STAT1S");
    HealthBarX += 56.0;
    DrawBarSprite(Info, "STAT2S");
    HealthBarX += 56.0;
    DrawBarSprite(Info, "STAT3S");
    HealthBarX += 56.0;
    DrawBarSprite(Info, "STAT4S");
    HealthBarX = -96.0;
    HealthBarY += 20.0;
    DrawBarSprite(Info, "STAT5S");
    HealthBarX += 56.0;
    DrawBarSprite(Info, "STAT6S");
    HealthBarX += 56.0;
    DrawBarSprite(Info, "STAT7S");
    HealthBarX += 56.0;
    DrawBarSprite(Info, "STAT8S");
}
