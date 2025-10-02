@tool
extends EditorPlugin

func _enter_tree():
	print("GodotEOS: Plugin activated")
	# Add autoload when plugin is enabled
	add_autoload_singleton("EpicOS", "res://addons/godoteos/epic_os.gd")

func _exit_tree():
	print("GodotEOS: Plugin deactivated")
	# Remove autoload when plugin is disabled
	remove_autoload_singleton("EpicOS")