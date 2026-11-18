module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"
#include <array>
#include <string>
#include <string_view>
#include <unordered_map>

export module Prompts;

import Gamepad;

// Hints that name keyboard keys or mouse buttons get controller wording while the controller is the last device used.
// The game looks every hint up again each frame it is drawn, so the wording follows the device at once.
namespace FarCryPromptsState
{
    enum Language { English, German, French, Italian, Spanish, Russian, Polish, Japanese, LanguageCount };

    using Texts = std::array<const wchar_t*, LanguageCount>;

    enum Phrase
    {
        ObjectivesHint, ThrowGrenades, GrenadeType, Sprint, EnterVehicles, MoveForward, Jump, LieDown, StandUp, Ladders,
        UseThem, Flashlight, Zoom, Aim, HeatVision, Shoot, Reload, MountedGun, AimingMode, FiringRates, ReloadOut, Look,
        MoveBackward, Sidestep, Lean, Sneak, Rock, Machete, ThirdPerson, OpenDoors, DriveForward, Perspective, FiringMode,
        DisableAlarm, PullLever, InsertKeycard, ElevatorDown, CallElevator, ElevatorUp, PlantExplosives, OpenDoor, DropWeapon,
        UseBinoculars, ZoomBinoculars, DriveForklift, DriveHumvee, DriveBuggy, DriveTruck, DrivePatrolBoat, DriveInflatable,
        FlyGlider, GunHumvee, GunBoat, RideHumvee, RideInflatable, RideTruck, RideBuggy, UseMortar, UseMachineGun, ToggleView,
        LeaveFlyingFox, PushBoat, ActivateLift, OpenCages, CloseSteamValve, UseElevator, UseFlyingFox, OpenGate, GoDown, GoUp,
        ActivateDoor, DeactivateLock, PlantBomb, PressButton, ElevatorAccess, TankDoor, SeaWater, MoveDown, MoveUp,
        CallElevatorDown, CallElevatorUp, Objectives, ExitBoat, OpenValve, PullSwitch, PowerSupply, OperateComputer,
        StartGenerator, UploadPda, ActivateElevator, PhraseCount
    };

    // English, German, French, Italian, Spanish, Russian, Polish, Japanese
    const Texts Phrases[PhraseCount] =
    {
        Texts{ L"To display your current objectives, hold {BACK}", L"Halte {BACK} gedrückt, um deine aktuellen Ziele anzuzeigen", L"Maintenez {BACK} pour afficher vos objectifs actuels", L"Tieni premuto {BACK} per vedere gli obiettivi attuali", L"Mantén {BACK} para ver tus objetivos actuales", L"Удерживайте {BACK}, чтобы увидеть текущие задачи", L"Przytrzymaj {BACK}, aby zobaczyć bieżące cele", L"{BACK}長押しで現在の目標を表示" },
        Texts{ L"Press {RB} to throw grenades", L"Drücke {RB}, um Granaten zu werfen", L"Appuyez sur {RB} pour lancer des grenades", L"Premi {RB} per lanciare granate", L"Pulsa {RB} para lanzar granadas", L"Нажмите {RB}, чтобы бросить гранату", L"Naciśnij {RB}, aby rzucić granat", L"{RB}でグレネードを投げる" },
        Texts{ L"Hold {Y} to change the grenade type", L"Halte {Y} gedrückt, um die Granatenart zu wechseln", L"Maintenez {Y} pour changer de type de grenade", L"Tieni premuto {Y} per cambiare tipo di granata", L"Mantén {Y} para cambiar el tipo de granada", L"Удерживайте {Y}, чтобы сменить тип гранат", L"Przytrzymaj {Y}, aby zmienić rodzaj granatów", L"{Y}長押しでグレネードの種類を切り替え" },
        Texts{ L"Press {LS} while moving to sprint", L"Drücke {LS} während der Bewegung, um zu sprinten", L"Appuyez sur {LS} en vous déplaçant pour sprinter", L"Premi {LS} mentre ti muovi per scattare", L"Pulsa {LS} mientras te mueves para esprintar", L"Нажмите {LS} во время движения, чтобы бежать быстрее", L"Naciśnij {LS} podczas ruchu, aby biec sprintem", L"移動中に{LS}でダッシュ" },
        Texts{ L"Hold {X} to enter vehicles", L"Halte {X} gedrückt, um in Fahrzeuge einzusteigen", L"Maintenez {X} pour monter dans les véhicules", L"Tieni premuto {X} per salire sui veicoli", L"Mantén {X} para subir a los vehículos", L"Удерживайте {X}, чтобы сесть в транспорт", L"Przytrzymaj {X}, aby wsiąść do pojazdu", L"{X}長押しで乗り物に乗る" },
        Texts{ L"Push the left stick forward to move forward", L"Drücke den linken Stick nach vorn, um vorwärts zu gehen", L"Poussez le stick gauche vers l'avant pour avancer", L"Spingi in avanti la levetta sinistra per avanzare", L"Mueve el stick izquierdo hacia delante para avanzar", L"Наклоните левый стик вперёд, чтобы идти вперёд", L"Wychyl lewą gałkę do przodu, aby iść naprzód", L"左スティックを前に倒して前進" },
        Texts{ L"Press {A} to jump up", L"Drücke {A}, um zu springen", L"Appuyez sur {A} pour sauter", L"Premi {A} per saltare", L"Pulsa {A} para saltar", L"Нажмите {A}, чтобы прыгнуть", L"Naciśnij {A}, aby skoczyć", L"{A}でジャンプ" },
        Texts{ L"Hold {B} to lie down", L"Halte {B} gedrückt, um dich hinzulegen", L"Maintenez {B} pour vous allonger", L"Tieni premuto {B} per sdraiarti", L"Mantén {B} para tumbarte", L"Удерживайте {B}, чтобы лечь", L"Przytrzymaj {B}, aby się położyć", L"{B}長押しで伏せる" },
        Texts{ L"Hold {B} again to stand up", L"Halte {B} erneut gedrückt, um aufzustehen", L"Maintenez de nouveau {B} pour vous relever", L"Tieni di nuovo premuto {B} per rialzarti", L"Vuelve a mantener {B} para levantarte", L"Снова удерживайте {B}, чтобы встать", L"Ponownie przytrzymaj {B}, aby wstać", L"もう一度{B}長押しで立ち上がる" },
        Texts{ L"Look up and push the left stick forward to climb ladders", L"Schau nach oben und drücke den linken Stick nach vorn, um Leitern hochzuklettern", L"Regardez vers le haut et poussez le stick gauche vers l'avant pour monter aux échelles", L"Guarda in alto e spingi in avanti la levetta sinistra per salire le scale", L"Mira hacia arriba y mueve el stick izquierdo hacia delante para subir escaleras", L"Посмотрите вверх и наклоните левый стик вперёд, чтобы лезть по лестнице", L"Spójrz w górę i wychyl lewą gałkę do przodu, aby wspinać się po drabinie", L"上を向いて左スティックを前に倒すとはしごを登る" },
        Texts{ L"Press {LB} to use them", L"Drücke {LB}, um es zu benutzen", L"Appuyez sur {LB} pour les utiliser", L"Premi {LB} per usarlo", L"Pulsa {LB} para usarlos", L"Нажмите {LB}, чтобы воспользоваться им", L"Naciśnij {LB}, aby ich użyć", L"{LB}で使用" },
        Texts{ L"Press {DUP} to turn your flashlight on or off", L"Drücke {DUP}, um die Taschenlampe ein- oder auszuschalten", L"Appuyez sur {DUP} pour allumer ou éteindre votre lampe", L"Premi {DUP} per accendere o spegnere la torcia", L"Pulsa {DUP} para encender o apagar la linterna", L"Нажмите {DUP}, чтобы включить или выключить фонарик", L"Naciśnij {DUP}, aby włączyć lub wyłączyć latarkę", L"{DUP}でライトのオン/オフ" },
        Texts{ L"Use {DUP} and {DDOWN} to zoom in and out", L"Mit {DUP} und {DDOWN} zoomst du hinein und heraus", L"Utilisez {DUP} et {DDOWN} pour zoomer et dézoomer", L"Usa {DUP} e {DDOWN} per aumentare e ridurre lo zoom", L"Usa {DUP} y {DDOWN} para acercar y alejar el zoom", L"{DUP} и {DDOWN} приближают и отдаляют изображение", L"Użyj {DUP} i {DDOWN}, aby przybliżyć i oddalić", L"{DUP}と{DDOWN}でズームイン/アウト" },
        Texts{ L"Press {LT} to aim your weapon", L"Drücke {LT}, um mit der Waffe zu zielen", L"Appuyez sur {LT} pour viser avec votre arme", L"Premi {LT} per mirare con l'arma", L"Pulsa {LT} para apuntar con el arma", L"Нажмите {LT}, чтобы прицелиться", L"Naciśnij {LT}, aby wycelować bronią", L"{LT}で武器を構える" },
        Texts{ L"and press {LT} again to switch to heat vision", L"und drücke erneut {LT}, um zur Wärmesicht zu wechseln", L"et appuyez de nouveau sur {LT} pour passer en vision thermique", L"e premi di nuovo {LT} per passare alla visione termica", L"y vuelve a pulsar {LT} para cambiar a visión térmica", L"и снова нажмите {LT}, чтобы включить тепловизор", L"i ponownie naciśnij {LT}, aby włączyć termowizję", L"もう一度{LT}で熱感知モードに切り替え" },
        Texts{ L"Press {RT} to shoot", L"Drücke {RT}, um zu schießen", L"Appuyez sur {RT} pour tirer", L"Premi {RT} per sparare", L"Pulsa {RT} para disparar", L"Нажмите {RT}, чтобы стрелять", L"Naciśnij {RT}, aby strzelać", L"{RT}で射撃" },
        Texts{ L"Press {X} to reload your current weapon", L"Drücke {X}, um deine Waffe nachzuladen", L"Appuyez sur {X} pour recharger votre arme", L"Premi {X} per ricaricare l'arma", L"Pulsa {X} para recargar el arma", L"Нажмите {X}, чтобы перезарядить оружие", L"Naciśnij {X}, aby przeładować broń", L"{X}でリロード" },
        Texts{ L"Go to the mounted machine gun and hold {X} to use it", L"Geh zum montierten Maschinengewehr und halte {X} gedrückt, um es zu benutzen", L"Allez à la mitrailleuse fixe et maintenez {X} pour l'utiliser", L"Raggiungi la mitragliatrice fissa e tieni premuto {X} per usarla", L"Ve a la ametralladora fija y mantén {X} para usarla", L"Подойдите к стационарному пулемёту и удерживайте {X}, чтобы воспользоваться им", L"Podejdź do stacjonarnego karabinu maszynowego i przytrzymaj {X}, aby go użyć", L"据え付けの機関銃に近づき{X}長押しで使用" },
        Texts{ L"Hold {LT} to enable the aiming mode", L"Halte {LT} gedrückt, um den Zielmodus zu aktivieren", L"Maintenez {LT} pour activer le mode de visée", L"Tieni premuto {LT} per attivare la modalità di mira", L"Mantén {LT} para activar el modo de apuntado", L"Удерживайте {LT}, чтобы включить режим прицеливания", L"Przytrzymaj {LT}, aby włączyć tryb celowania", L"{LT}長押しで照準モード" },
        Texts{ L"Press {RS} to select firing rates", L"Drücke {RS}, um die Feuerrate zu wählen", L"Appuyez sur {RS} pour choisir la cadence de tir", L"Premi {RS} per scegliere la cadenza di fuoco", L"Pulsa {RS} para elegir la cadencia de tiro", L"Нажмите {RS}, чтобы выбрать режим стрельбы", L"Naciśnij {RS}, aby wybrać tryb ognia", L"{RS}で射撃モードを選択" },
        Texts{ L"Press {X} to reload your weapon, if you run out of", L"Drücke {X}, um deine Waffe nachzuladen. Wenn dir die", L"Appuyez sur {X} pour recharger votre arme. Si vous n'avez plus de", L"Premi {X} per ricaricare l'arma. Se finisci i", L"Pulsa {X} para recargar el arma. Si te quedas sin", L"Нажмите {X}, чтобы перезарядить оружие. Если кончатся", L"Naciśnij {X}, aby przeładować broń. Jeśli skończą ci się", L"{X}でリロード。弾が尽きたら" },
        Texts{ L"Use the right stick to look around", L"Benutze den rechten Stick, um dich umzusehen", L"Utilisez le stick droit pour regarder autour de vous", L"Usa la levetta destra per guardarti intorno", L"Usa el stick derecho para mirar alrededor", L"Осматривайтесь правым стиком", L"Rozglądaj się prawą gałką", L"右スティックで周りを見る" },
        Texts{ L"Pull the left stick back to move backward", L"Ziehe den linken Stick zurück, um rückwärts zu gehen", L"Tirez le stick gauche vers l'arrière pour reculer", L"Tira indietro la levetta sinistra per indietreggiare", L"Mueve el stick izquierdo hacia atrás para retroceder", L"Наклоните левый стик назад, чтобы идти назад", L"Wychyl lewą gałkę do tyłu, aby się cofać", L"左スティックを後ろに倒して後退" },
        Texts{ L"Push the left stick left or right to sidestep", L"Drücke den linken Stick nach links oder rechts, um seitwärts zu gehen", L"Poussez le stick gauche à gauche ou à droite pour faire un pas de côté", L"Spingi la levetta sinistra a sinistra o a destra per spostarti di lato", L"Mueve el stick izquierdo a izquierda o derecha para moverte de lado", L"Наклоните левый стик влево или вправо, чтобы идти боком", L"Wychyl lewą gałkę w lewo lub w prawo, aby iść bokiem", L"左スティックを左右に倒して横移動" },
        Texts{ L"Press {DLEFT} or {DRIGHT} to lean around corners", L"Drücke {DLEFT} oder {DRIGHT}, um dich um Ecken zu lehnen", L"Appuyez sur {DLEFT} ou {DRIGHT} pour vous pencher aux angles", L"Premi {DLEFT} o {DRIGHT} per sporgerti dagli angoli", L"Pulsa {DLEFT} o {DRIGHT} para asomarte por las esquinas", L"Нажмите {DLEFT} или {DRIGHT}, чтобы выглянуть из-за угла", L"Naciśnij {DLEFT} lub {DRIGHT}, aby wychylić się zza rogu", L"{DLEFT}か{DRIGHT}で角からのぞき込む" },
        Texts{ L"Press {B} to crouch, then push the left stick forward to sneak", L"Drücke {B}, um in die Hocke zu gehen, und drücke dann den linken Stick nach vorn, um zu schleichen", L"Appuyez sur {B} pour vous accroupir, puis poussez le stick gauche vers l'avant pour avancer discrètement", L"Premi {B} per accovacciarti, poi spingi in avanti la levetta sinistra per avanzare furtivamente", L"Pulsa {B} para agacharte y mueve el stick izquierdo hacia delante para avanzar con sigilo", L"Нажмите {B}, чтобы присесть, и наклоните левый стик вперёд, чтобы красться", L"Naciśnij {B}, aby kucnąć, i wychyl lewą gałkę do przodu, aby się skradać", L"{B}でしゃがみ、左スティックを前に倒して忍び足" },
        Texts{ L"Press {RB} to throw a rock and distract your enemies", L"Drücke {RB}, um einen Stein zu werfen und deine Feinde abzulenken", L"Appuyez sur {RB} pour lancer une pierre et distraire vos ennemis", L"Premi {RB} per lanciare un sasso e distrarre i nemici", L"Pulsa {RB} para lanzar una piedra y distraer a tus enemigos", L"Нажмите {RB}, чтобы бросить камень и отвлечь врагов", L"Naciśnij {RB}, aby rzucić kamień i odwrócić uwagę wrogów", L"{RB}で石を投げて敵の気をそらす" },
        Texts{ L"Press {Y} to switch your current weapon to the machete", L"Drücke {Y}, um zur Machete zu wechseln", L"Appuyez sur {Y} pour passer à la machette", L"Premi {Y} per passare al machete", L"Pulsa {Y} para cambiar al machete", L"Нажмите {Y}, чтобы взять мачете", L"Naciśnij {Y}, aby wybrać maczetę", L"{Y}でマチェーテに持ち替える" },
        Texts{ L"Tip: Press {Y} to switch to third person view", L"Tipp: Drücke {Y}, um zur Third-Person-Ansicht zu wechseln", L"Astuce : appuyez sur {Y} pour passer à la vue à la troisième personne", L"Suggerimento: premi {Y} per passare alla visuale in terza persona", L"Consejo: pulsa {Y} para cambiar a la vista en tercera persona", L"Совет: нажмите {Y}, чтобы перейти к виду от третьего лица", L"Wskazówka: naciśnij {Y}, aby przełączyć na widok z trzeciej osoby", L"ヒント: {Y}で三人称視点に切り替え" },
        Texts{ L"Hold {X} to open doors", L"Halte {X} gedrückt, um Türen zu öffnen", L"Maintenez {X} pour ouvrir les portes", L"Tieni premuto {X} per aprire le porte", L"Mantén {X} para abrir puertas", L"Удерживайте {X}, чтобы открывать двери", L"Przytrzymaj {X}, aby otwierać drzwi", L"{X}長押しでドアを開ける" },
        Texts{ L"Push the left stick forward to drive forward", L"Drücke den linken Stick nach vorn, um vorwärts zu fahren", L"Poussez le stick gauche vers l'avant pour avancer", L"Spingi in avanti la levetta sinistra per andare avanti", L"Mueve el stick izquierdo hacia delante para conducir hacia delante", L"Наклоните левый стик вперёд, чтобы ехать вперёд", L"Wychyl lewą gałkę do przodu, aby jechać naprzód", L"左スティックを前に倒して前進" },
        Texts{ L"Press {Y} to change perspective", L"Drücke {Y}, um die Perspektive zu wechseln", L"Appuyez sur {Y} pour changer de point de vue", L"Premi {Y} per cambiare visuale", L"Pulsa {Y} para cambiar la perspectiva", L"Нажмите {Y}, чтобы сменить вид", L"Naciśnij {Y}, aby zmienić widok", L"{Y}で視点を切り替え" },
        Texts{ L"Press {RS} to change firing mode", L"Drücke {RS}, um den Feuermodus zu wechseln", L"Appuyez sur {RS} pour changer de mode de tir", L"Premi {RS} per cambiare modalità di fuoco", L"Pulsa {RS} para cambiar el modo de disparo", L"Нажмите {RS}, чтобы сменить режим стрельбы", L"Naciśnij {RS}, aby zmienić tryb ognia", L"{RS}で射撃モードを切り替え" },
        Texts{ L"Hold {X} to disable the alarm", L"Halte {X} gedrückt, um den Alarm abzuschalten", L"Maintenez {X} pour désactiver l'alarme", L"Tieni premuto {X} per disattivare l'allarme", L"Mantén {X} para desactivar la alarma", L"Удерживайте {X}, чтобы отключить тревогу", L"Przytrzymaj {X}, aby wyłączyć alarm", L"{X}長押しで警報を解除" },
        Texts{ L"Hold {X} to pull the lever", L"Halte {X} gedrückt, um den Hebel zu ziehen", L"Maintenez {X} pour tirer le levier", L"Tieni premuto {X} per tirare la leva", L"Mantén {X} para tirar de la palanca", L"Удерживайте {X}, чтобы потянуть рычаг", L"Przytrzymaj {X}, aby pociągnąć dźwignię", L"{X}長押しでレバーを引く" },
        Texts{ L"Hold {X} to insert the keycard", L"Halte {X} gedrückt, um die Schlüsselkarte einzustecken", L"Maintenez {X} pour insérer la carte d'accès", L"Tieni premuto {X} per inserire la tessera", L"Mantén {X} para introducir la tarjeta", L"Удерживайте {X}, чтобы вставить ключ-карту", L"Przytrzymaj {X}, aby włożyć kartę", L"{X}長押しでカードキーを差し込む" },
        Texts{ L"Hold {X} to move the elevator down", L"Halte {X} gedrückt, um den Aufzug nach unten zu fahren", L"Maintenez {X} pour faire descendre l'ascenseur", L"Tieni premuto {X} per far scendere l'ascensore", L"Mantén {X} para bajar el ascensor", L"Удерживайте {X}, чтобы опустить лифт", L"Przytrzymaj {X}, aby zjechać windą w dół", L"{X}長押しでエレベーターを下げる" },
        Texts{ L"Hold {X} to call the elevator", L"Halte {X} gedrückt, um den Aufzug zu rufen", L"Maintenez {X} pour appeler l'ascenseur", L"Tieni premuto {X} per chiamare l'ascensore", L"Mantén {X} para llamar al ascensor", L"Удерживайте {X}, чтобы вызвать лифт", L"Przytrzymaj {X}, aby przywołać windę", L"{X}長押しでエレベーターを呼ぶ" },
        Texts{ L"Hold {X} to move the elevator up", L"Halte {X} gedrückt, um den Aufzug nach oben zu fahren", L"Maintenez {X} pour faire monter l'ascenseur", L"Tieni premuto {X} per far salire l'ascensore", L"Mantén {X} para subir el ascensor", L"Удерживайте {X}, чтобы поднять лифт", L"Przytrzymaj {X}, aby wjechać windą w górę", L"{X}長押しでエレベーターを上げる" },
        Texts{ L"Hold {X} to plant explosives here", L"Halte {X} gedrückt, um hier Sprengstoff anzubringen", L"Maintenez {X} pour poser des explosifs ici", L"Tieni premuto {X} per piazzare qui gli esplosivi", L"Mantén {X} para colocar explosivos aquí", L"Удерживайте {X}, чтобы заложить здесь взрывчатку", L"Przytrzymaj {X}, aby podłożyć tu ładunek", L"{X}長押しでここに爆薬を仕掛ける" },
        Texts{ L"Hold {X} to open the door", L"Halte {X} gedrückt, um die Tür zu öffnen", L"Maintenez {X} pour ouvrir la porte", L"Tieni premuto {X} per aprire la porta", L"Mantén {X} para abrir la puerta", L"Удерживайте {X}, чтобы открыть дверь", L"Przytrzymaj {X}, aby otworzyć drzwi", L"{X}長押しでドアを開ける" },
        Texts{ L"Hold {RS} to drop the", L"Halte {RS} gedrückt, um Folgendes abzulegen:", L"Maintenez {RS} pour lâcher :", L"Tieni premuto {RS} per lasciare:", L"Mantén {RS} para soltar:", L"Удерживайте {RS}, чтобы выбросить:", L"Przytrzymaj {RS}, aby upuścić:", L"{RS}長押しで捨てる:" },
        Texts{ L"Press {LB} to use the binoculars", L"Drücke {LB}, um das Fernglas zu benutzen", L"Appuyez sur {LB} pour utiliser les jumelles", L"Premi {LB} per usare il binocolo", L"Pulsa {LB} para usar los prismáticos", L"Нажмите {LB}, чтобы воспользоваться биноклем", L"Naciśnij {LB}, aby użyć lornetki", L"{LB}で双眼鏡を使う" },
        Texts{ L"Zoom in and out with {DUP} and {DDOWN}", L"Mit {DUP} und {DDOWN} zoomst du hinein und heraus", L"Zoomez et dézoomez avec {DUP} et {DDOWN}", L"Aumenta e riduci lo zoom con {DUP} e {DDOWN}", L"Acerca y aleja el zoom con {DUP} y {DDOWN}", L"Приближайте и отдаляйте с помощью {DUP} и {DDOWN}", L"Przybliżaj i oddalaj za pomocą {DUP} i {DDOWN}", L"{DUP}と{DDOWN}でズームイン/アウト" },
        Texts{ L"Hold {X} to drive the forklift", L"Halte {X} gedrückt, um den Gabelstapler zu fahren", L"Maintenez {X} pour conduire le chariot élévateur", L"Tieni premuto {X} per guidare il muletto", L"Mantén {X} para conducir la carretilla elevadora", L"Удерживайте {X}, чтобы сесть за руль погрузчика", L"Przytrzymaj {X}, aby kierować wózkiem widłowym", L"{X}長押しでフォークリフトを運転" },
        Texts{ L"Hold {X} to drive the 4WD vehicle", L"Halte {X} gedrückt, um den Geländewagen zu fahren", L"Maintenez {X} pour conduire le 4x4", L"Tieni premuto {X} per guidare il fuoristrada", L"Mantén {X} para conducir el todoterreno", L"Удерживайте {X}, чтобы сесть за руль внедорожника", L"Przytrzymaj {X}, aby kierować terenówką", L"{X}長押しで4WD車を運転" },
        Texts{ L"Hold {X} to drive the buggy", L"Halte {X} gedrückt, um den Buggy zu fahren", L"Maintenez {X} pour conduire le buggy", L"Tieni premuto {X} per guidare il buggy", L"Mantén {X} para conducir el buggy", L"Удерживайте {X}, чтобы сесть за руль багги", L"Przytrzymaj {X}, aby kierować buggy", L"{X}長押しでバギーを運転" },
        Texts{ L"Hold {X} to drive the big truck", L"Halte {X} gedrückt, um den Lastwagen zu fahren", L"Maintenez {X} pour conduire le camion", L"Tieni premuto {X} per guidare il camion", L"Mantén {X} para conducir el camión", L"Удерживайте {X}, чтобы сесть за руль грузовика", L"Przytrzymaj {X}, aby kierować ciężarówką", L"{X}長押しでトラックを運転" },
        Texts{ L"Hold {X} to drive the patrol boat", L"Halte {X} gedrückt, um das Patrouillenboot zu fahren", L"Maintenez {X} pour piloter le patrouilleur", L"Tieni premuto {X} per guidare la motovedetta", L"Mantén {X} para pilotar la lancha patrullera", L"Удерживайте {X}, чтобы управлять патрульным катером", L"Przytrzymaj {X}, aby sterować łodzią patrolową", L"{X}長押しで巡視艇を操縦" },
        Texts{ L"Hold {X} to drive the inflatable boat", L"Halte {X} gedrückt, um das Schlauchboot zu fahren", L"Maintenez {X} pour piloter le canot pneumatique", L"Tieni premuto {X} per guidare il gommone", L"Mantén {X} para pilotar la lancha neumática", L"Удерживайте {X}, чтобы управлять надувной лодкой", L"Przytrzymaj {X}, aby sterować pontonem", L"{X}長押しでゴムボートを操縦" },
        Texts{ L"Hold {X} to fly the hang glider", L"Halte {X} gedrückt, um mit dem Drachen zu fliegen", L"Maintenez {X} pour piloter le deltaplane", L"Tieni premuto {X} per pilotare il deltaplano", L"Mantén {X} para volar en el ala delta", L"Удерживайте {X}, чтобы лететь на дельтаплане", L"Przytrzymaj {X}, aby lecieć lotnią", L"{X}長押しでハンググライダーに乗る" },
        Texts{ L"Hold {X} to take the gun of the 4WD vehicle", L"Halte {X} gedrückt, um das Geschütz des Geländewagens zu bedienen", L"Maintenez {X} pour prendre l'arme du 4x4", L"Tieni premuto {X} per usare l'arma del fuoristrada", L"Mantén {X} para manejar el arma del todoterreno", L"Удерживайте {X}, чтобы встать за пулемёт внедорожника", L"Przytrzymaj {X}, aby obsadzić działko terenówki", L"{X}長押しで4WD車の銃座につく" },
        Texts{ L"Hold {X} to take the gun of the patrol boat", L"Halte {X} gedrückt, um das Geschütz des Patrouillenboots zu bedienen", L"Maintenez {X} pour prendre l'arme du patrouilleur", L"Tieni premuto {X} per usare l'arma della motovedetta", L"Mantén {X} para manejar el arma de la lancha patrullera", L"Удерживайте {X}, чтобы встать за пулемёт патрульного катера", L"Przytrzymaj {X}, aby obsadzić działko łodzi patrolowej", L"{X}長押しで巡視艇の銃座につく" },
        Texts{ L"Hold {X} to ride in the 4WD vehicle", L"Halte {X} gedrückt, um im Geländewagen mitzufahren", L"Maintenez {X} pour monter à bord du 4x4", L"Tieni premuto {X} per salire sul fuoristrada", L"Mantén {X} para subir al todoterreno", L"Удерживайте {X}, чтобы сесть во внедорожник пассажиром", L"Przytrzymaj {X}, aby jechać terenówką jako pasażer", L"{X}長押しで4WD車に同乗" },
        Texts{ L"Hold {X} to ride in the inflatable boat", L"Halte {X} gedrückt, um im Schlauchboot mitzufahren", L"Maintenez {X} pour monter à bord du canot pneumatique", L"Tieni premuto {X} per salire sul gommone", L"Mantén {X} para subir a la lancha neumática", L"Удерживайте {X}, чтобы сесть в надувную лодку пассажиром", L"Przytrzymaj {X}, aby płynąć pontonem jako pasażer", L"{X}長押しでゴムボートに同乗" },
        Texts{ L"Hold {X} to ride in the big truck", L"Halte {X} gedrückt, um im Lastwagen mitzufahren", L"Maintenez {X} pour monter à bord du camion", L"Tieni premuto {X} per salire sul camion", L"Mantén {X} para subir al camión", L"Удерживайте {X}, чтобы сесть в грузовик пассажиром", L"Przytrzymaj {X}, aby jechać ciężarówką jako pasażer", L"{X}長押しでトラックに同乗" },
        Texts{ L"Hold {X} to ride in the buggy", L"Halte {X} gedrückt, um im Buggy mitzufahren", L"Maintenez {X} pour monter à bord du buggy", L"Tieni premuto {X} per salire sul buggy", L"Mantén {X} para subir al buggy", L"Удерживайте {X}, чтобы сесть в багги пассажиром", L"Przytrzymaj {X}, aby jechać buggy jako pasażer", L"{X}長押しでバギーに同乗" },
        Texts{ L"Hold {X} to use the mortar", L"Halte {X} gedrückt, um den Mörser zu benutzen", L"Maintenez {X} pour utiliser le mortier", L"Tieni premuto {X} per usare il mortaio", L"Mantén {X} para usar el mortero", L"Удерживайте {X}, чтобы воспользоваться миномётом", L"Przytrzymaj {X}, aby użyć moździerza", L"{X}長押しで迫撃砲を使う" },
        Texts{ L"Hold {X} to use the machine gun", L"Halte {X} gedrückt, um das Maschinengewehr zu benutzen", L"Maintenez {X} pour utiliser la mitrailleuse", L"Tieni premuto {X} per usare la mitragliatrice", L"Mantén {X} para usar la ametralladora", L"Удерживайте {X}, чтобы воспользоваться пулемётом", L"Przytrzymaj {X}, aby użyć karabinu maszynowego", L"{X}長押しで機関銃を使う" },
        Texts{ L"Press {Y} to change the view", L"Drücke {Y}, um die Ansicht zu wechseln", L"Appuyez sur {Y} pour changer de vue", L"Premi {Y} per cambiare visuale", L"Pulsa {Y} para cambiar la vista", L"Нажмите {Y}, чтобы сменить вид", L"Naciśnij {Y}, aby zmienić widok", L"{Y}で視点を切り替え" },
        Texts{ L"Hold {X} to jump off the flying fox", L"Halte {X} gedrückt, um von der Seilrutsche zu springen", L"Maintenez {X} pour sauter de la tyrolienne", L"Tieni premuto {X} per saltare giù dalla teleferica", L"Mantén {X} para saltar de la tirolina", L"Удерживайте {X}, чтобы спрыгнуть с троса", L"Przytrzymaj {X}, aby zeskoczyć z tyrolki", L"{X}長押しでジップラインから飛び降りる" },
        Texts{ L"Hold {X} to push the boat into the water", L"Halte {X} gedrückt, um das Boot ins Wasser zu schieben", L"Maintenez {X} pour pousser le bateau à l'eau", L"Tieni premuto {X} per spingere la barca in acqua", L"Mantén {X} para empujar el bote al agua", L"Удерживайте {X}, чтобы столкнуть лодку в воду", L"Przytrzymaj {X}, aby zepchnąć łódź do wody", L"{X}長押しでボートを水に押し出す" },
        Texts{ L"Hold {X} to activate the lift", L"Halte {X} gedrückt, um den Lift zu aktivieren", L"Maintenez {X} pour activer le monte-charge", L"Tieni premuto {X} per attivare il montacarichi", L"Mantén {X} para activar el montacargas", L"Удерживайте {X}, чтобы включить подъёмник", L"Przytrzymaj {X}, aby uruchomić podnośnik", L"{X}長押しでリフトを作動" },
        Texts{ L"Hold {X} to open the cages from the console", L"Halte {X} gedrückt, um die Käfige über die Konsole zu öffnen", L"Maintenez {X} pour ouvrir les cages depuis la console", L"Tieni premuto {X} per aprire le gabbie dalla console", L"Mantén {X} para abrir las jaulas desde la consola", L"Удерживайте {X}, чтобы открыть клетки с пульта", L"Przytrzymaj {X}, aby otworzyć klatki z konsoli", L"{X}長押しでコンソールから檻を開ける" },
        Texts{ L"Hold {X} to close the steam valve", L"Halte {X} gedrückt, um das Dampfventil zu schließen", L"Maintenez {X} pour fermer la vanne de vapeur", L"Tieni premuto {X} per chiudere la valvola del vapore", L"Mantén {X} para cerrar la válvula de vapor", L"Удерживайте {X}, чтобы закрыть паровой вентиль", L"Przytrzymaj {X}, aby zamknąć zawór pary", L"{X}長押しで蒸気バルブを閉める" },
        Texts{ L"Hold {X} to use the elevator", L"Halte {X} gedrückt, um den Aufzug zu benutzen", L"Maintenez {X} pour utiliser l'ascenseur", L"Tieni premuto {X} per usare l'ascensore", L"Mantén {X} para usar el ascensor", L"Удерживайте {X}, чтобы воспользоваться лифтом", L"Przytrzymaj {X}, aby skorzystać z windy", L"{X}長押しでエレベーターを使う" },
        Texts{ L"Hold {X} to use the flying fox", L"Halte {X} gedrückt, um die Seilrutsche zu benutzen", L"Maintenez {X} pour utiliser la tyrolienne", L"Tieni premuto {X} per usare la teleferica", L"Mantén {X} para usar la tirolina", L"Удерживайте {X}, чтобы спуститься по тросу", L"Przytrzymaj {X}, aby skorzystać z tyrolki", L"{X}長押しでジップラインを使う" },
        Texts{ L"Hold {X} to open the gate", L"Halte {X} gedrückt, um das Tor zu öffnen", L"Maintenez {X} pour ouvrir le portail", L"Tieni premuto {X} per aprire il cancello", L"Mantén {X} para abrir la verja", L"Удерживайте {X}, чтобы открыть ворота", L"Przytrzymaj {X}, aby otworzyć bramę", L"{X}長押しでゲートを開ける" },
        Texts{ L"Hold {X} to go down", L"Halte {X} gedrückt, um nach unten zu fahren", L"Maintenez {X} pour descendre", L"Tieni premuto {X} per scendere", L"Mantén {X} para bajar", L"Удерживайте {X}, чтобы спуститься", L"Przytrzymaj {X}, aby zjechać w dół", L"{X}長押しで下へ" },
        Texts{ L"Hold {X} to go up", L"Halte {X} gedrückt, um nach oben zu fahren", L"Maintenez {X} pour monter", L"Tieni premuto {X} per salire", L"Mantén {X} para subir", L"Удерживайте {X}, чтобы подняться", L"Przytrzymaj {X}, aby wjechać w górę", L"{X}長押しで上へ" },
        Texts{ L"Hold {X} to activate the door", L"Halte {X} gedrückt, um die Tür zu aktivieren", L"Maintenez {X} pour actionner la porte", L"Tieni premuto {X} per azionare la porta", L"Mantén {X} para accionar la puerta", L"Удерживайте {X}, чтобы открыть дверь", L"Przytrzymaj {X}, aby uruchomić drzwi", L"{X}長押しでドアを作動" },
        Texts{ L"Hold {X} to deactivate the security lock", L"Halte {X} gedrückt, um die Sicherheitssperre aufzuheben", L"Maintenez {X} pour désactiver le verrou de sécurité", L"Tieni premuto {X} per disattivare il blocco di sicurezza", L"Mantén {X} para desactivar el bloqueo de seguridad", L"Удерживайте {X}, чтобы снять защитную блокировку", L"Przytrzymaj {X}, aby wyłączyć blokadę bezpieczeństwa", L"{X}長押しでセキュリティロックを解除" },
        Texts{ L"Hold {X} to plant the bomb", L"Halte {X} gedrückt, um die Bombe zu legen", L"Maintenez {X} pour poser la bombe", L"Tieni premuto {X} per piazzare la bomba", L"Mantén {X} para colocar la bomba", L"Удерживайте {X}, чтобы заложить бомбу", L"Przytrzymaj {X}, aby podłożyć bombę", L"{X}長押しで爆弾を仕掛ける" },
        Texts{ L"Hold {X} to press the button", L"Halte {X} gedrückt, um den Knopf zu drücken", L"Maintenez {X} pour appuyer sur le bouton", L"Tieni premuto {X} per premere il pulsante", L"Mantén {X} para pulsar el botón", L"Удерживайте {X}, чтобы нажать кнопку", L"Przytrzymaj {X}, aby nacisnąć przycisk", L"{X}長押しでボタンを押す" },
        Texts{ L"Hold {X} to enable access to the elevator", L"Halte {X} gedrückt, um den Zugang zum Aufzug freizugeben", L"Maintenez {X} pour activer l'accès à l'ascenseur", L"Tieni premuto {X} per abilitare l'accesso all'ascensore", L"Mantén {X} para habilitar el acceso al ascensor", L"Удерживайте {X}, чтобы открыть доступ к лифту", L"Przytrzymaj {X}, aby odblokować dostęp do windy", L"{X}長押しでエレベーターを使用可能にする" },
        Texts{ L"Hold {X} to open the big door to the tank rooms", L"Halte {X} gedrückt, um das große Tor zu den Tankräumen zu öffnen", L"Maintenez {X} pour ouvrir la grande porte des salles des cuves", L"Tieni premuto {X} per aprire la grande porta delle sale delle vasche", L"Mantén {X} para abrir la gran puerta de las salas de tanques", L"Удерживайте {X}, чтобы открыть большую дверь в залы с резервуарами", L"Przytrzymaj {X}, aby otworzyć wielkie drzwi do sal ze zbiornikami", L"{X}長押しでタンク室への大扉を開ける" },
        Texts{ L"Hold {X} to let sea water into the complex", L"Halte {X} gedrückt, um Meerwasser in den Komplex zu lassen", L"Maintenez {X} pour laisser entrer l'eau de mer dans le complexe", L"Tieni premuto {X} per far entrare l'acqua di mare nel complesso", L"Mantén {X} para dejar entrar agua de mar en el complejo", L"Удерживайте {X}, чтобы впустить морскую воду в комплекс", L"Przytrzymaj {X}, aby wpuścić wodę morską do kompleksu", L"{X}長押しで施設に海水を入れる" },
        Texts{ L"Hold {X} to move down", L"Halte {X} gedrückt, um nach unten zu fahren", L"Maintenez {X} pour descendre", L"Tieni premuto {X} per scendere", L"Mantén {X} para bajar", L"Удерживайте {X}, чтобы спуститься", L"Przytrzymaj {X}, aby zjechać w dół", L"{X}長押しで下へ" },
        Texts{ L"Hold {X} to move up", L"Halte {X} gedrückt, um nach oben zu fahren", L"Maintenez {X} pour monter", L"Tieni premuto {X} per salire", L"Mantén {X} para subir", L"Удерживайте {X}, чтобы подняться", L"Przytrzymaj {X}, aby wjechać w górę", L"{X}長押しで上へ" },
        Texts{ L"Hold {X} to call the elevator down", L"Halte {X} gedrückt, um den Aufzug nach unten zu rufen", L"Maintenez {X} pour appeler l'ascenseur en bas", L"Tieni premuto {X} per chiamare giù l'ascensore", L"Mantén {X} para llamar al ascensor abajo", L"Удерживайте {X}, чтобы вызвать лифт вниз", L"Przytrzymaj {X}, aby przywołać windę na dół", L"{X}長押しでエレベーターを下に呼ぶ" },
        Texts{ L"Hold {X} to call the elevator up", L"Halte {X} gedrückt, um den Aufzug nach oben zu rufen", L"Maintenez {X} pour appeler l'ascenseur en haut", L"Tieni premuto {X} per chiamare su l'ascensore", L"Mantén {X} para llamar al ascensor arriba", L"Удерживайте {X}, чтобы вызвать лифт наверх", L"Przytrzymaj {X}, aby przywołać windę na górę", L"{X}長押しでエレベーターを上に呼ぶ" },
        Texts{ L"Hold {BACK} to see your mission objectives", L"Halte {BACK} gedrückt, um deine Missionsziele zu sehen", L"Maintenez {BACK} pour voir vos objectifs de mission", L"Tieni premuto {BACK} per vedere gli obiettivi della missione", L"Mantén {BACK} para ver los objetivos de la misión", L"Удерживайте {BACK}, чтобы увидеть задачи миссии", L"Przytrzymaj {BACK}, aby zobaczyć cele misji", L"{BACK}長押しでミッション目標を表示" },
        Texts{ L"Hold {X} to exit the boat", L"Halte {X} gedrückt, um aus dem Boot zu steigen", L"Maintenez {X} pour descendre du bateau", L"Tieni premuto {X} per scendere dalla barca", L"Mantén {X} para bajar del bote", L"Удерживайте {X}, чтобы выйти из лодки", L"Przytrzymaj {X}, aby wysiąść z łodzi", L"{X}長押しでボートから降りる" },
        Texts{ L"Hold {X} to open the valve", L"Halte {X} gedrückt, um das Ventil zu öffnen", L"Maintenez {X} pour ouvrir la vanne", L"Tieni premuto {X} per aprire la valvola", L"Mantén {X} para abrir la válvula", L"Удерживайте {X}, чтобы открыть вентиль", L"Przytrzymaj {X}, aby otworzyć zawór", L"{X}長押しでバルブを開ける" },
        Texts{ L"Hold {X} to pull the switch", L"Halte {X} gedrückt, um den Schalter umzulegen", L"Maintenez {X} pour actionner l'interrupteur", L"Tieni premuto {X} per azionare l'interruttore", L"Mantén {X} para accionar el interruptor", L"Удерживайте {X}, чтобы переключить рубильник", L"Przytrzymaj {X}, aby przestawić przełącznik", L"{X}長押しでスイッチを入れる" },
        Texts{ L"Hold {X} to turn on the power supply", L"Halte {X} gedrückt, um die Stromversorgung einzuschalten", L"Maintenez {X} pour rétablir l'alimentation électrique", L"Tieni premuto {X} per attivare l'alimentazione elettrica", L"Mantén {X} para activar el suministro eléctrico", L"Удерживайте {X}, чтобы включить электропитание", L"Przytrzymaj {X}, aby włączyć zasilanie", L"{X}長押しで電源を入れる" },
        Texts{ L"Hold {X} to operate the computer", L"Halte {X} gedrückt, um den Computer zu bedienen", L"Maintenez {X} pour utiliser l'ordinateur", L"Tieni premuto {X} per usare il computer", L"Mantén {X} para usar el ordenador", L"Удерживайте {X}, чтобы воспользоваться компьютером", L"Przytrzymaj {X}, aby obsłużyć komputer", L"{X}長押しでコンピューターを操作" },
        Texts{ L"Hold {X} to start the generator.", L"Halte {X} gedrückt, um den Generator zu starten.", L"Maintenez {X} pour démarrer le générateur.", L"Tieni premuto {X} per avviare il generatore.", L"Mantén {X} para arrancar el generador.", L"Удерживайте {X}, чтобы запустить генератор.", L"Przytrzymaj {X}, aby uruchomić generator.", L"{X}長押しで発電機を起動。" },
        Texts{ L"Hold {X} to upload the PDA", L"Halte {X} gedrückt, um den PDA hochzuladen", L"Maintenez {X} pour transférer le PDA", L"Tieni premuto {X} per caricare il PDA", L"Mantén {X} para cargar la PDA", L"Удерживайте {X}, чтобы загрузить данные КПК", L"Przytrzymaj {X}, aby przesłać dane z palmtopa", L"{X}長押しでPDAをアップロード" },
        Texts{ L"Hold {X} to activate the elevator", L"Halte {X} gedrückt, um den Aufzug zu aktivieren", L"Maintenez {X} pour activer l'ascenseur", L"Tieni premuto {X} per attivare l'ascensore", L"Mantén {X} para activar el ascensor", L"Удерживайте {X}, чтобы включить лифт", L"Przytrzymaj {X}, aby uruchomić windę", L"{X}長押しでエレベーターを作動" },
    };

    // Lower-case string keys of the hints that name keys, buttons or the mouse
    const std::pair<const char*, Phrase> Keys[] =
    {
        { "hinttodisplayyou", ObjectivesHint }, { "pressgtothrowgre", ThrowGrenades }, { "presshtotogglegr", GrenadeType },
        { "holddownzandanym", Sprint }, { "presshkeytochang", GrenadeType }, { "pressfkeytoenter", EnterVehicles },
        { "holddownctrlkeya", MoveForward }, { "pressspacekeytoj", Jump }, { "pressvkeytoprone", LieDown },
        { "pressvkeyagainto", StandUp }, { "lookupandpressfo", Ladders }, { "pressbtousethem", UseThem },
        { "pressftouseyourf", Flashlight }, { "zoominoutwiththe", Zoom }, { "pressrightmouseb", Aim },
        { "andpressrightmou", HeatVision }, { "pressleftmousebu", Shoot }, { "pressrtoreload", Reload },
        { "gotothemountedma", MountedGun }, { "holddowntheright", AimingMode }, { "pressxtoswitchto", FiringRates },
        { "pressrtoreloadyo", ReloadOut }, { "useyourmousetolo", Look }, { "presswtomoveforw", MoveForward },
        { "pressstomoveback", MoveBackward }, { "pressaanddtostra", Sidestep }, { "useqandekeystole", Lean },
        { "holddownctrlandw", Sneak }, { "throwtherocktodi", Rock }, { "press2toswitchyo", Machete },
        { "tippressrightmou", ThirdPerson }, { "pressfkeytoopend", OpenDoors }, { "holddownthesprin", Sprint },
        { "pressforwardkeyd", DriveForward }, { "pressf1keytochan", Perspective }, { "pressleftmousebu", Shoot },
        { "pressleftmousebx", FiringMode }, { "disable_alarm", DisableAlarm }, { "pull_lever", PullLever },
        { "insert_keycard", InsertKeycard }, { "move_elevator_down", ElevatorDown }, { "call_elevator", CallElevator },
        { "move_elevator_up", ElevatorUp }, { "plant_explosives_here", PlantExplosives }, { "opendoor", OpenDoor },
        { "pressdropweapon", DropWeapon }, { "bino1", UseBinoculars }, { "bino2", ZoomBinoculars },
        { "driverforklift", DriveForklift }, { "driverhumvee", DriveHumvee }, { "driverbuggy", DriveBuggy },
        { "driverbigtruck", DriveTruck }, { "driverboat", DrivePatrolBoat }, { "driverzodiac", DriveInflatable },
        { "driverparaglider", FlyGlider }, { "gunnerhumvee", GunHumvee }, { "gunnerboat", GunBoat },
        { "passengerhumvee", RideHumvee }, { "passengerzodiac", RideInflatable }, { "passengerbigtruck", RideTruck },
        { "passengerbuggy", RideBuggy }, { "usemortar", UseMortar }, { "usemachinegun", UseMachineGun },
        { "usemountedweaponvehicle", UseMachineGun }, { "switchviewpara", ToggleView }, { "drivefox", LeaveFlyingFox },
        { "pushboat", PushBoat }, { "bunker_lift", ActivateLift }, { "bynker_use_console", OpenCages },
        { "bunker_open_door", OpenDoor }, { "closevalve", CloseSteamValve }, { "dam_call_elevator", CallElevator },
        { "dam_use_elevator", UseElevator }, { "flyingfox", UseFlyingFox }, { "open_gate", OpenGate },
        { "archive_call_elevator1", CallElevator }, { "archive_call_elevator2", GoDown }, { "archive_call_elevator3", GoUp },
        { "archive_call_elevator4", GoDown }, { "archive_button1", ActivateDoor }, { "deactivatesecurit", DeactivateLock },
        { "open_door", OpenDoor }, { "pressuseexpboat", PlantBomb }, { "plant_bomb", PlantBomb },
        { "compound_open_door", OpenDoor }, { "compound_press_button", PressButton }, { "cooler_switch1", ElevatorAccess },
        { "cooler_switch2", TankDoor }, { "cooler_switch3", SeaWater }, { "cooler_elevator1", CallElevator },
        { "cooler_elevator2", MoveDown }, { "cooler_elevator3", MoveUp }, { "fort_elevator_down", CallElevatorDown },
        { "fort_elevator_up", CallElevatorUp }, { "fort_plant_bomb", PlantBomb }, { "presstabto", Objectives },
        { "pressuseboat", ExitBoat }, { "pressusetoopenva", OpenValve }, { "pressusetopull", PullSwitch },
        { "pier_plant_bomb", PlantBomb }, { "regulator_open_door", OpenDoor }, { "regulator_power_on", PowerSupply },
        { "regulator_operate_computer", OperateComputer }, { "regulator_disable_alarm", DisableAlarm },
        { "regulator_use_valve", CloseSteamValve }, { "start_generator", StartGenerator }, { "upload_pda", UploadPda },
        { "treehouse.callelevator", CallElevator }, { "vulcano_activate_elevator", ActivateElevator },
        { "vulcano_use_elevator", UseElevator }, { "vulcano_open_door", OpenDoor },
    };

    // D-pad directions: up, down, left, right
    const std::array<const wchar_t*, 4> DPad[LanguageCount] =
    {
        std::array{ L"D-PAD UP", L"D-PAD DOWN", L"D-PAD LEFT", L"D-PAD RIGHT" },
        std::array{ L"STEUERKREUZ OBEN", L"STEUERKREUZ UNTEN", L"STEUERKREUZ LINKS", L"STEUERKREUZ RECHTS" },
        std::array{ L"CROIX HAUT", L"CROIX BAS", L"CROIX GAUCHE", L"CROIX DROITE" },
        std::array{ L"CROCE SU", L"CROCE GIÙ", L"CROCE SINISTRA", L"CROCE DESTRA" },
        std::array{ L"CRUCETA ARRIBA", L"CRUCETA ABAJO", L"CRUCETA IZQUIERDA", L"CRUCETA DERECHA" },
        std::array{ L"КРЕСТОВИНА ВВЕРХ", L"КРЕСТОВИНА ВНИЗ", L"КРЕСТОВИНА ВЛЕВО", L"КРЕСТОВИНА ВПРАВО" },
        std::array{ L"KRZYŻAK W GÓRĘ", L"KRZYŻAK W DÓŁ", L"KRZYŻAK W LEWO", L"KRZYŻAK W PRAWO" },
        std::array{ L"十字キー上", L"十字キー下", L"十字キー左", L"十字キー右" },
    };

    // Shoulders, triggers, stick buttons: LB, RB, LT, RT, LS, RS
    const std::array<const wchar_t*, 6> Shoulders[3] =
    {
        std::array{ L"LB", L"RB", L"LT", L"RT", L"LS", L"RS" },
        std::array{ L"L1", L"R1", L"L2", L"R2", L"L3", L"R3" },
        std::array{ L"L", L"R", L"ZL", L"ZR", L"LS", L"RS" },
    };

    std::unordered_map<std::string_view, Phrase> Lookup;
    std::wstring Cache[PhraseCount];
    int CacheLanguage = -1, CacheFamily = -1;
    SafetyHookInline LookupHook;

#ifdef _WIN64
    constexpr size_t languageOffset = 96;
    void* (__fastcall* AssignWide)(void* string, const wchar_t* text, size_t length) = nullptr;
#else
    constexpr size_t languageOffset = 40;
    struct WideRange
    {
        const wchar_t* begin;
        const wchar_t* end;
        const wchar_t* capacity;
    };
    void* (__fastcall* AssignWide)(void* string, void* edx, const WideRange* source) = nullptr;
#endif

    // The string table keeps the game language it was loaded for
    int GameLanguage(void* table)
    {
        const auto base = static_cast<uint8_t*>(table) + languageOffset;
#ifdef _WIN64
        const auto capacity = *reinterpret_cast<size_t*>(base + 32);
        const auto size = *reinterpret_cast<size_t*>(base + 24);
        const auto text = capacity >= 16 ? *reinterpret_cast<const char**>(base + 8) : reinterpret_cast<const char*>(base + 8);
#else
        const auto text = *reinterpret_cast<const char**>(base);
        const auto size = size_t(*reinterpret_cast<const char**>(base + 4) - text);
#endif
        const std::string_view name(text ? text : "", text ? size : 0);
        static constexpr std::pair<std::string_view, Language> names[] =
        {
            { "english", English }, { "german", German }, { "french", French }, { "italian", Italian },
            { "spanish", Spanish }, { "russian", Russian }, { "polish", Polish }, { "japanese", Japanese },
        };
        for (const auto& [languageName, language] : names)
        {
            if (name.size() == languageName.size() && !_strnicmp(name.data(), languageName.data(), name.size()))
                return language;
        }
        return English;
    }

    std::wstring Format(const wchar_t* text, int language, PadInput::Family family)
    {
        const int set = int(family) - 1;
        std::wstring result;
        for (auto c = text; *c; ++c)
        {
            if (*c != L'{')
            {
                result += *c;
                continue;
            }
            const auto end = std::wcschr(c, L'}');
            const std::wstring_view name(c + 1, size_t(end - c - 1));
            if (name == L"A")
                result += PadInput::ButtonName(XINPUT_GAMEPAD_A, family);
            else if (name == L"B")
                result += PadInput::ButtonName(XINPUT_GAMEPAD_B, family);
            else if (name == L"X")
                result += PadInput::ButtonName(XINPUT_GAMEPAD_X, family);
            else if (name == L"Y")
                result += PadInput::ButtonName(XINPUT_GAMEPAD_Y, family);
            else if (name == L"BACK")
                result += PadInput::ButtonName(XINPUT_GAMEPAD_BACK, family);
            else if (name == L"START")
                result += PadInput::ButtonName(XINPUT_GAMEPAD_START, family);
            else if (name == L"LB")
                result += Shoulders[set][0];
            else if (name == L"RB")
                result += Shoulders[set][1];
            else if (name == L"LT")
                result += Shoulders[set][2];
            else if (name == L"RT")
                result += Shoulders[set][3];
            else if (name == L"LS")
                result += Shoulders[set][4];
            else if (name == L"RS")
                result += Shoulders[set][5];
            else if (name == L"DUP")
                result += DPad[language][0];
            else if (name == L"DDOWN")
                result += DPad[language][1];
            else if (name == L"DLEFT")
                result += DPad[language][2];
            else if (name == L"DRIGHT")
                result += DPad[language][3];
            c = end;
        }
        return result;
    }

    const std::wstring* PadText(void* table, const char* key)
    {
        if (!key || !FarCryPad::PadInUse())
            return nullptr;
        if (*key == '@')
            ++key;
        char lower[64]{};
        size_t length = 0;
        for (; key[length] && key[length] != ' '; ++length)
        {
            if (length + 1 >= sizeof(lower))
                return nullptr;
            lower[length] = char(tolower(uint8_t(key[length])));
        }
        const auto found = Lookup.find(std::string_view(lower, length));
        if (found == Lookup.end())
            return nullptr;
        const int language = GameLanguage(table);
        const auto family = PadInput::ResolveFamily(FixSettings().Int("MAIN.GamepadIcons"));
        if (language != CacheLanguage || int(family) != CacheFamily)
        {
            for (int i = 0; i < PhraseCount; ++i)
                Cache[i] = Format(Phrases[i][language], language, family);
            CacheLanguage = language;
            CacheFamily = int(family);
        }
        return &Cache[found->second];
    }

#ifdef _WIN64
    bool LookupString(void* table, const char* key, void* text)
    {
        if (auto pad = PadText(table, key))
        {
            AssignWide(text, pad->c_str(), pad->size());
            return true;
        }
        return LookupHook.call<bool>(table, key, text);
    }
#else
    bool __fastcall LookupString(void* table, void* edx, const char* key, void* text)
    {
        if (auto pad = PadText(table, key))
        {
            const WideRange range{ pad->c_str(), pad->c_str() + pad->size(), pad->c_str() + pad->size() };
            AssignWide(text, nullptr, &range);
            return true;
        }
        return LookupHook.thiscall<bool>(table, key, text);
    }
#endif
}

export namespace FarCryPrompts
{
    inline void Init()
    {
        if (FarCryPromptsState::LookupHook)
            return;
        for (const auto& [key, phrase] : FarCryPromptsState::Keys)
            FarCryPromptsState::Lookup.emplace(key, phrase);
        auto module = GetModuleHandleW(L"CryGame.dll");
#ifdef _WIN64
        auto lookup = hook::module_pattern(module, "48 8B C4 48 81 EC 68 02 00 00 48 C7 44 24 50 FE FF FF FF 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 49 8B D8 48 8B F2 48 8B E9 80 3A 40 0F 85 ? ? ? ? 48 8B CA 48 8D 54 24 60 48 2B D1 0F B6 01 88 04 11 48 FF C1 84 C0 75 F3 48 8D 4C 24 60 FF 15 ? ? ? ? 41 B8 FF FF FF FF 48 8D 54 24 60 48 8D 4D 48 E8 ? ? ? ? 85 C0 78 6B 48 98 48 8D 0C 80 48 8B 45 10").get_first();
        FarCryPromptsState::AssignWide = reinterpret_cast<decltype(FarCryPromptsState::AssignWide)>(injector::GetBranchDestination(static_cast<uint8_t*>(lookup) + 0xCF).as_int());
#else
        auto lookup = hook::module_pattern(module, "B8 ? ? ? ? E8 ? ? ? ? 81 EC 18 02 00 00 53 8B 5D 08 80 3B 40 56 57 8B F9 0F 85 ? ? ? ? 8D 95 DC FD FF FF 8B C3 2B D3 8A 08 88 0C 02 40 84 C9 75 F6 8D 85 DC FD FF FF 50 FF 15 ? ? ? ? 59 6A FF 83 EC 0C 8B CC 89 65 08 8D 45 0F 50 8D 85 DC FD FF FF 50 E8 ? ? ? ? 8D 77 1C 56 E8 ? ? ? ? 83 C4 14 85 C0 7D 27 6A FF 83 EC 0C 8B CC 89 65 08 8D 45 0F 50 8D 85 DD FD FF FF 50 E8 ? ? ? ? 56 E8 ? ? ? ? 83 C4 14 85 C0 7C 16 8B 4F 04").get_first();
        FarCryPromptsState::AssignWide = reinterpret_cast<decltype(FarCryPromptsState::AssignWide)>(injector::GetBranchDestination(static_cast<uint8_t*>(lookup) + 0xA1).as_int());
#endif
        FarCryPromptsState::LookupHook = safetyhook::create_inline(lookup, &FarCryPromptsState::LookupString);
    }
}
