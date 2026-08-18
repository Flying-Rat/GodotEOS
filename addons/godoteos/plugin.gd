@tool
extends EditorPlugin

func _log(message: String) -> void:
	print("[GodotEOS] EpicOS: ", message)

func _enter_tree():
	_log("Plugin activated")
	# Add autoload when plugin is enabled
	add_autoload_singleton("EpicOS", "res://addons/godoteos/epic_os.gd")

func _exit_tree():
	_log("Plugin deactivated")
	# Remove autoload when plugin is disabled
	remove_autoload_singleton("EpicOS")
