@tool
extends EditorPlugin

func _enter_tree():
	print("GodotEpic: Plugin activated")
	# Add autoload when plugin is enabled
	add_autoload_singleton("EpicOS", "res://addons/godot_epic/epic_os.gd")

func _exit_tree():
	print("GodotEpic: Plugin deactivated")
	# Remove autoload when plugin is disabled
	remove_autoload_singleton("EpicOS")