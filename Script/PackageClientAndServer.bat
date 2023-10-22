@echo off

set ENGINE_PATH=C:\UE_4.27_Source
set PROJECT_PATH=C:\Users\sauli\OneDrive\Documentos\Unreal Projects\Git\MultiplayerPhaaS_DedicatedServer
set CLIENT_GAME_INSTANCE_CLASS=/Game/Blueprints/Gameplay/GameInstances/BouncingSpheres/GI_BouncingSpheres_Client.GI_BouncingSpheres_Client_C
set SERVER_GAME_INSTANCE_CLASS=/Game/Blueprints/Gameplay/GameInstances/BouncingSpheres/GI_BouncingSpheres_Server.GI_BouncingSpheres_Server_C

:: Create a backup of the DefaultEngine.ini
copy /Y "%PROJECT_PATH%\Config\DefaultEngine.ini" "%PROJECT_PATH%\Config\DefaultEngine.backup.ini"

:: Find and replace the GameInstanceClass setting in DefaultEngine.ini
findstr /v /i "GameInstanceClass" "%PROJECT_PATH%\Config\DefaultEngine.ini" >  "%PROJECT_PATH%\Config\DefaultEngine.tmp.ini"
echo. >> "%PROJECT_PATH%\Config\DefaultEngine.tmp.ini"
echo [/Script/EngineSettings.GameMapsSettings] >> "%PROJECT_PATH%\Config\DefaultEngine.tmp.ini"
echo GameInstanceClass=%CLIENT_GAME_INSTANCE_CLASS% >> "%PROJECT_PATH%\Config\DefaultEngine.tmp.ini"

:: Replace the original file with the modified file
move /y "%PROJECT_PATH%\Config\DefaultEngine.tmp.ini" "%PROJECT_PATH%\Config\DefaultEngine.ini"

:: Build your project with the modified GameInstanceClass
call %ENGINE_PATH%\Engine\Build\BatchFiles\RunUAT.bat -ScriptsForProject="%PROJECT_PATH%/MultiplayerPhaaS.uproject" BuildCookRun -nocompileeditor -nop4 -project="%PROJECT_PATH%/MultiplayerPhaaS.uproject" -cook -stage -archive -archivedirectory="%PROJECT_PATH%/Build" -package -ue4exe=C:\UE_4.27_Source\Engine\Binaries\Win64\UE4Editor-Cmd.exe -compressed -ddc=DerivedDataBackendGraph -pak -prereqs -nodebuginfo -targetplatform=Win64 -build -target=MultiplayerPhaaS -clientconfig=Development -utf8output -compile

echo "Restoring backup of DefaultEngine.ini..."

:: Restore the original DefaultEngine.ini (optional)
move /y "%PROJECT_PATH%\Config\DefaultEngine.backup.ini" "%PROJECT_PATH%\Config\DefaultEngine.ini"

:: Create a shortcut with -log appended to the destination path
set "shortcutTarget=%PROJECT_PATH%\Build\WindowsNoEditor\MultiplayerPhaaS.exe"
set "shortcutPath=%PROJECT_PATH%\Build\WindowsNoEditor\MultiplayerPhaaS-log.lnk"
set "arguments=-log"

powershell -Command "$WshShell = New-Object -ComObject WScript.Shell; $Shortcut = $WshShell.CreateShortcut('%shortcutPath%'); $Shortcut.TargetPath = '%shortcutTarget%'; $Shortcut.Arguments = '%arguments%'; $Shortcut.Save()"

echo "Building server..."

:: Create a backup of the DefaultEngine.ini
copy /Y "%PROJECT_PATH%\Config\DefaultEngine.ini" "%PROJECT_PATH%\Config\DefaultEngine.backup.ini"

:: Find and replace the GameInstanceClass setting in DefaultEngine.ini
findstr /v /i "GameInstanceClass" "%PROJECT_PATH%\Config\DefaultEngine.ini" >  "%PROJECT_PATH%\Config\DefaultEngine.tmp.ini"
echo. >> "%PROJECT_PATH%\Config\DefaultEngine.tmp.ini"
echo [/Script/EngineSettings.GameMapsSettings] >> "%PROJECT_PATH%\Config\DefaultEngine.tmp.ini"
echo GameInstanceClass=%SERVER_GAME_INSTANCE_CLASS% >> "%PROJECT_PATH%\Config\DefaultEngine.tmp.ini"

:: Replace the original file with the modified file
move /y "%PROJECT_PATH%\Config\DefaultEngine.tmp.ini" "%PROJECT_PATH%\Config\DefaultEngine.ini"

:: Build your project with the modified GameInstanceClass
call %ENGINE_PATH%\Engine\Build\BatchFiles\RunUAT.bat -ScriptsForProject="%PROJECT_PATH%/MultiplayerPhaaS.uproject" BuildCookRun -nocompileeditor -nop4 -project="%PROJECT_PATH%/MultiplayerPhaaS.uproject" -cook -stage -archive -archivedirectory="%PROJECT_PATH%/Build" -package -ue4exe=C:\UE_4.27_Source\Engine\Binaries\Win64\UE4Editor-Cmd.exe -compressed -ddc=DerivedDataBackendGraph -pak -prereqs -nodebuginfo -targetplatform=Win64 -build -target=MultiplayerPhaaSServer -serverconfig=Development -utf8output -compile

echo "Restoring backup of DefaultEngine.ini..."

:: Restore the original DefaultEngine.ini (optional)
move /y "%PROJECT_PATH%\Config\DefaultEngine.backup.ini" "%PROJECT_PATH%\Config\DefaultEngine.ini"

:: Create a shortcut with -log appended to the destination path
set "shortcutTarget=%PROJECT_PATH%\Build\WindowsServer\MultiplayerPhaaSServer.exe"
set "shortcutPath=%PROJECT_PATH%\Build\WindowsServer\MultiplayerPhaaSServer-log.lnk"
set "arguments=-log"

powershell -Command "$WshShell = New-Object -ComObject WScript.Shell; $Shortcut = $WshShell.CreateShortcut('%shortcutPath%'); $Shortcut.TargetPath = '%shortcutTarget%'; $Shortcut.Arguments = '%arguments%'; $Shortcut.Save()"
