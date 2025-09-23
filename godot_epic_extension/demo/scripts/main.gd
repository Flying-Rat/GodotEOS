extends Node2D

var godot_epic: GodotEpic = null

# Called when the node enters the scene tree for the first time.
func _ready():
	godot_epic = GodotEpic.get_singleton()
	
	# Example initialization options for Epic Online Services
	var init_options = {
		"product_name": "GodotEpic Demo",
		"product_version": "1.0.0",
		"product_id": "your_product_id_here",
		"sandbox_id": "your_sandbox_id_here", 
		"deployment_id": "your_deployment_id_here",
		"client_id": "your_client_id_here",
		"client_secret": "your_client_secret_here",
		"encryption_key": "your_encryption_key_here"  # Optional but recommended
	}
	
	# Initialize the EOS platform
	var success = godot_epic.initialize_platform(init_options)
	if success:
		print("EOS Platform initialized successfully!")
	else:
		print("Failed to initialize EOS Platform. Check your credentials.")
	
	# Check if platform is initialized
	if godot_epic.is_platform_initialized():
		print("EOS Platform is ready to use.")


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	# Tick the EOS platform to handle callbacks and updates
	if godot_epic and godot_epic.is_platform_initialized():
		godot_epic.tick()


# Called when the node is about to be removed from the scene
func _exit_tree():
	# Clean shutdown of EOS platform
	if godot_epic:
		godot_epic.shutdown_platform()
		print("EOS Platform shut down.")
