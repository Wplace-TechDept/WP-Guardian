import json
import os

def load_module_settings():
    """
    C++アプリケーションのsettings.jsonからModuleSettingsを取得する。
    settings.jsonはWP_Guardian.exeと同じディレクトリにあると仮定する。
    Pythonスクリプトがassets/modules/Python/以下にいることを前提に、
    settings.jsonのパスを推測する。
    """
    # 現在のスクリプトのディレクトリを取得
    script_dir = os.path.dirname(__file__)
    # WP-Guardianのプロジェクトルートを推測（assets/modules/Python から3つ上）
    app_root_dir = os.path.abspath(os.path.join(script_dir, "..", "..", ".."))
    
    settings_json_path = None
    exe_dir = None

    # プロジェクトルート以下を探索してWP_Guardian.exeを探す
    for root, dirs, files in os.walk(app_root_dir):
        if "WP_Guardian.exe" in files:
            exe_dir = root
            break
    
    if exe_dir:
        settings_json_path = os.path.join(exe_dir, "settings.json")
        if not os.path.exists(settings_json_path):
            print(f"WARNING: WP_Guardian.exe found at {exe_dir}, but settings.json not found in that directory. Trying project root.")
            settings_json_path = None # 見つからなければプロジェクトルート直下を再試行するためリセット

    # WP_Guardian.exe のディレクトリで見つからなかった場合、プロジェクトルート直下も試す (保険)
    if settings_json_path is None:
        if os.path.exists(os.path.join(app_root_dir, "settings.json")):
            settings_json_path = os.path.join(app_root_dir, "settings.json")


    default_ip = "127.0.0.1"
    default_port = 9002

    if settings_json_path is None:
        print(f"WARNING: settings.json not found in WP_Guardian.exe's directory or project root. Using default IP/Port.")
        return default_ip, default_port

    print(f"INFO: Loading module settings from {settings_json_path}")

    try:
        with open(settings_json_path, "r", encoding="utf-8") as f:
            settings = json.load(f)
            
            module_settings = settings.get("ModuleSettings", {})
            
            server_ip = module_settings.get("serverIP", default_ip)
            server_port = module_settings.get("serverPort", default_port)
            
            return server_ip, server_port
            
    except json.JSONDecodeError:
        print(f"ERROR: Failed to parse settings.json at {settings_json_path}. Using default IP/Port.")
        return default_ip, default_port
    except Exception as e:
        print(f"ERROR: An unexpected error occurred while loading settings.json: {e}. Using default IP/Port.")
        return default_ip, default_port


