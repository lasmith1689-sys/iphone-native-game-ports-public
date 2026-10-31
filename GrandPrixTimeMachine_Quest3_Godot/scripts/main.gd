extends Node3D
## Grand Prix Time Machine — Fuji 1976 standalone OpenXR experience.
## Historical BEATS are documented. Geometry, cars and playback are illustrative.
## v0.7: historical six-wheel car, entrant numbers, and safer controller activation.

const Track = preload("res://scripts/track.gd")
const LENGTH := 123.0
const BEATS := [
    {"at": 0.0, "title": "FUJI 1976", "subtitle": "24 OCTOBER  |  73 LAPS  |  CHAMPIONSHIP DECIDER", "detail": "Lauda arrives three points ahead of Hunt. Heavy rain threatens the race."},
    {"at": 14.0, "title": "THE START", "subtitle": "HUNT TAKES THE EARLY LEAD", "detail": "Andretti starts on pole. Hunt moves ahead in treacherous conditions."},
    {"at": 30.0, "title": "LAUDA WITHDRAWS", "subtitle": "AFTER TWO LAPS  |  SAFETY FIRST", "detail": "Lauda stops in the pits: the torrential conditions are too dangerous."},
    {"at": 58.0, "title": "LATE-RACE DRAMA", "subtitle": "HUNT PITS  |  TYRE TROUBLE", "detail": "The track dries. Late tyre trouble sends Hunt to the pits and down the order."},
    {"at": 83.0, "title": "THE FINISH", "subtitle": "ANDRETTI 1ST  |  DEPAILLER 2ND  |  HUNT 3RD", "detail": "Andretti wins. Hunt recovers to third place, enough for the title."},
    {"at": 104.0, "title": "WORLD CHAMPION", "subtitle": "HUNT 69 POINTS  |  LAUDA 68", "detail": "James Hunt wins the 1976 World Drivers' Championship by one point."}
]
const COLORS := {
    "charcoal": Color(0.065, 0.083, 0.10),
    "grass": Color(0.10, 0.21, 0.17),
    "road": Color(0.11, 0.14, 0.17),
    "kerb": Color(0.92, 0.19, 0.10),
    "cream": Color(0.89, 0.88, 0.79),
    "gold": Color(0.96, 0.70, 0.25)
}

@onready var xr_camera: XRCamera3D = $XROrigin3D/XRCamera3D
@onready var desktop_camera: Camera3D = $DesktopPreviewCamera
@onready var right_controller: XRController3D = $XROrigin3D/RightController
@onready var left_controller: XRController3D = $XROrigin3D/LeftController
var xr_interface: XRInterface
var xr_active := false
var passthrough_active := false
var board: Node3D
var ui_panel: Node3D
var track: GPTrack
var car_nodes: Dictionary = {}
var controls: Array[Dictionary] = []
var labels: Dictionary = {}
var cursor: Node3D
var playback := 0.0
var playing := true
var speed := 1.0
var chapter := -1
var expanded := false
var tabletop_scale := 0.94
var trigger_down := {"left": false, "right": false}
var center_down := {"left": false, "right": false}
var beams: Dictionary = {}
var hovered_action := ""
var intro_time := 0.0

func _ready() -> void:
    _make_environment()
    _build_diorama()
    _add_pointer(left_controller, "left")
    _add_pointer(right_controller, "right")
    _recenter()
    call_deferred("_start_xr")

func _start_xr() -> void:
    xr_interface = XRServer.find_interface("OpenXR")
    if xr_interface != null and xr_interface.initialize():
        xr_active = true
        get_viewport().use_xr = true
        desktop_camera.current = false
        print("GPTM XR session initialized")
        # On Meta Quest, passthrough also requires the OpenXR Vendors extension.
        # Without it, keep an immersive VR fallback rather than promising MR.
        if XRInterface.XR_ENV_BLEND_MODE_ALPHA_BLEND in xr_interface.get_supported_environment_blend_modes():
            passthrough_active = xr_interface.set_environment_blend_mode(XRInterface.XR_ENV_BLEND_MODE_ALPHA_BLEND)
            if passthrough_active:
                get_viewport().transparent_bg = true
                var environment := ($WorldEnvironment as WorldEnvironment).environment
                environment.background_mode = Environment.BG_COLOR
                environment.background_color = Color(0.0, 0.0, 0.0, 0.0)
        if not passthrough_active:
            xr_interface.set_environment_blend_mode(XRInterface.XR_ENV_BLEND_MODE_OPAQUE)
            get_viewport().transparent_bg = false
        print("GPTM passthrough active: ", passthrough_active)
    else:
        xr_active = false
        desktop_camera.current = true
        get_viewport().transparent_bg = false
        print("GPTM desktop preview — headset runtime unavailable")
    await get_tree().create_timer(0.3).timeout
    _recenter()

func _make_environment() -> void:
    var world := WorldEnvironment.new()
    world.name = "WorldEnvironment"
    var e := Environment.new()
    e.background_mode = Environment.BG_COLOR
    e.background_color = Color(0.018, 0.025, 0.037)
    e.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
    e.ambient_light_color = Color(0.75, 0.81, 0.89)
    e.ambient_light_energy = 1.0
    e.tonemap_mode = Environment.TONE_MAPPER_FILMIC
    world.environment = e
    add_child(world)
    var light := DirectionalLight3D.new()
    light.name = "Soft showroom light"
    light.light_energy = 1.35
    light.light_color = Color(1.0, 0.96, 0.88)
    light.rotation_degrees = Vector3(-50, -35, 0)
    add_child(light)

func _material(c: Color, rough: float = 0.75) -> StandardMaterial3D:
    var m := StandardMaterial3D.new()
    m.albedo_color = c
    m.roughness = rough
    return m

func _box(parent: Node3D, name: String, center: Vector3, size: Vector3, color: Color) -> MeshInstance3D:
    var m := MeshInstance3D.new()
    m.name = name
    var mesh := BoxMesh.new()
    mesh.size = size
    m.mesh = mesh
    m.material_override = _material(color)
    m.position = center
    parent.add_child(m)
    return m

func _sphere(parent: Node3D, name: String, center: Vector3, radius: float, color: Color) -> MeshInstance3D:
    var m := MeshInstance3D.new()
    m.name = name
    var mesh := SphereMesh.new()
    mesh.radius = radius
    mesh.height = radius * 2.0
    mesh.radial_segments = 12
    mesh.rings = 6
    m.mesh = mesh
    m.material_override = _material(color)
    m.position = center
    parent.add_child(m)
    return m

func _label(name: String, value: String, pos: Vector3, size: float, color: Color, right_align: bool = false, label_parent: Node3D = null) -> Label3D:
    var l := Label3D.new()
    l.name = name
    l.text = value
    l.font_size = 36
    l.pixel_size = size / 36.0
    l.modulate = color
    l.outline_size = 0
    l.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT if right_align else HORIZONTAL_ALIGNMENT_LEFT
    l.position = pos
    l.rotation.x = -PI / 2.0
    l.no_depth_test = true
    if label_parent == null:
        board.add_child(l)
    else:
        label_parent.add_child(l)
    labels[name] = l
    return l

func _track_surface(name: String, width: float, height: float, color: Color) -> void:
    var mesh := SurfaceTool.new()
    mesh.begin(Mesh.PRIMITIVE_TRIANGLES)
    for i in range(GPTrack.SAMPLE_COUNT):
        var a := float(i) / float(GPTrack.SAMPLE_COUNT)
        var b := float((i + 1) % GPTrack.SAMPLE_COUNT) / float(GPTrack.SAMPLE_COUNT)
        var pa: Vector3 = track.at(a)
        var pb: Vector3 = track.at(b)
        pa.y = height
        pb.y = height
        var ra: Vector3 = track.right(a) * (width / 2.0)
        var rb: Vector3 = track.right(b) * (width / 2.0)
        var v0: Vector3 = pa - ra
        var v1: Vector3 = pb - rb
        var v2: Vector3 = pa + ra
        var v3: Vector3 = pb + rb
        for v in [v0, v1, v2, v1, v3, v2]:
            mesh.set_normal(Vector3.UP)
            mesh.add_vertex(v)
    var inst := MeshInstance3D.new()
    inst.name = name
    inst.mesh = mesh.commit()
    # Damp pavement sheen without expensive full-screen reflections.
    inst.material_override = _material(color, 0.27 if name == "asphalt" else 0.82)
    board.add_child(inst)

func _pine_tree(center: Vector3, height: float) -> void:
    _box(board, "tree trunk", center + Vector3.UP * height * 0.17,
        Vector3(0.006, height * 0.34, 0.006), Color(0.20, 0.14, 0.08))
    for layer in range(3):
        var crown := MeshInstance3D.new()
        crown.name = "six-sided pine crown"
        var cone := CylinderMesh.new()
        cone.top_radius = 0.0
        cone.bottom_radius = 0.023 - 0.004 * layer
        cone.height = height * 0.47
        cone.radial_segments = 6
        crown.mesh = cone
        crown.material_override = _material(Color(0.07 + layer * 0.012, 0.24, 0.18))
        crown.position = center + Vector3.UP * height * (0.48 + 0.19 * layer)
        board.add_child(crown)

func _make_car(key: String, body_color: Color, accent_color: Color) -> void:
    var car := Node3D.new()
    car.name = key + " | illustrative 1976-style car"
    board.add_child(car)
    _box(car, "monocoque", Vector3(0, 0.012, 0), Vector3(0.028, 0.013, 0.071), body_color)
    _box(car, "nose", Vector3(0, 0.008, -0.036), Vector3(0.018, 0.007, 0.041), body_color)
    _box(car, "front wing", Vector3(0, 0.005, -0.059), Vector3(0.065, 0.004, 0.012), accent_color)
    _box(car, "rear wing", Vector3(0, 0.029, 0.046), Vector3(0.064, 0.005, 0.014), accent_color)
    _box(car, "rear wing mounts", Vector3(0, 0.018, 0.047), Vector3(0.005, 0.021, 0.006), body_color)
    _sphere(car, "driver helmet", Vector3(0, 0.025, 0.007), 0.010, Color(0.92, 0.88, 0.77))
    # Tyrrell P34: two small front axles, not a standard four-wheel car.
    var wheel_sites: Array[Vector2] = [
        Vector2(-0.029, -0.029), Vector2(0.029, -0.029),
        Vector2(-0.029, 0.028), Vector2(0.029, 0.028)
    ]
    if key == "DEPAILLER":
        wheel_sites = [
            Vector2(-0.028, -0.041), Vector2(0.028, -0.041),
            Vector2(-0.028, -0.018), Vector2(0.028, -0.018),
            Vector2(-0.030, 0.030), Vector2(0.030, 0.030)
        ]
    for site in wheel_sites:
        var z: float = site.y
        var tire := MeshInstance3D.new()
        tire.name = "front tire" if z < 0.0 else "rear tire"
        var cylinder := CylinderMesh.new()
        var front_radius := 0.007 if key == "DEPAILLER" else 0.009
        cylinder.top_radius = 0.011 if z > 0.0 else front_radius
        cylinder.bottom_radius = cylinder.top_radius
        cylinder.height = 0.013 if z > 0.0 else 0.009
        cylinder.radial_segments = 12
        tire.mesh = cylinder
        tire.material_override = _material(Color(0.025, 0.025, 0.03), 1.0)
        tire.position = Vector3(site.x, 0.010, z)
        tire.rotation.z = PI / 2.0
        car.add_child(tire)
    # Actual 1976 Fuji entrant numbers; original geometry, no copied decals.
    var race_numbers := {"ANDRETTI": "5", "HUNT": "11", "LAUDA": "1", "DEPAILLER": "4"}
    var number_label := Label3D.new()
    number_label.name = "historical car number"
    number_label.text = str(race_numbers.get(key, "?"))
    number_label.font_size = 36
    number_label.pixel_size = 0.00032
    number_label.outline_size = 0
    number_label.modulate = Color(0.05, 0.06, 0.09) if key == "HUNT" else Color.WHITE
    number_label.rotation.x = -PI / 2.0
    number_label.position = Vector3(0.0, 0.021, -0.027)
    car.add_child(number_label)
    car_nodes[key] = car

func _build_diorama() -> void:
    board = Node3D.new()
    board.name = "Fuji diorama"
    add_child(board)
    ui_panel = Node3D.new()
    ui_panel.name = "Headset-side playback controls"
    add_child(ui_panel)
    track = Track.new()
    _box(board, "museum base", Vector3(0.0, -0.024, 0.0), Vector3(1.29, 0.040, 0.91), COLORS.charcoal)
    _box(board, "lawn", Vector3(0, 0.000, 0.045), Vector3(1.24, 0.018, 0.78), COLORS.grass)
    _track_surface("red racing kerbs", GPTrack.ROAD_WIDTH + 0.016, 0.030, COLORS.kerb)
    _track_surface("asphalt", GPTrack.ROAD_WIDTH, 0.034, COLORS.road)
    _box(board, "pit lane", Vector3(-0.06, 0.027, -0.31), Vector3(0.64, 0.006, 0.052), COLORS.charcoal)
    _box(board, "pit building", Vector3(-0.065, 0.066, -0.348), Vector3(0.58, 0.06, 0.024), COLORS.cream)
    for i in range(8):
        var t := 0.008 + 0.007 * float(i)
        var p: Vector3 = track.at(t)
        p.y = 0.038
        var slot := _box(board, "grid", p, Vector3(0.035, 0.002, 0.002), COLORS.cream)
        slot.rotation.y = atan2(track.tangent(t).x, track.tangent(t).z)
    # Low-poly trees remain outside the car route.
    for side in [-1.0, 1.0]:
        for i in range(7):
            _pine_tree(Vector3(side * (0.594 + 0.004 * float(i % 2)), 0.011,
                -0.24 + 0.084 * float(i)), 0.068 + 0.013 * float(i % 3))
    for i in range(3):
        _box(board, "grandstand", Vector3(-0.22 + 0.2 * i, 0.047, 0.345), Vector3(0.17, 0.042, 0.034), COLORS.charcoal)
        for row in range(3):
            _box(board, "grandstand seating", Vector3(-0.22 + 0.2 * i, 0.065 + row * 0.007, 0.338 + row * 0.01), Vector3(0.155, 0.002, 0.008), COLORS.cream)
    _make_car("ANDRETTI", Color(0.13, 0.12, 0.11), Color(0.78, 0.64, 0.23))
    _make_car("HUNT", Color(0.96, 0.94, 0.91), Color(0.80, 0.16, 0.16))
    _make_car("LAUDA", Color(0.86, 0.10, 0.07), Color(0.99, 0.95, 0.87))
    _make_car("DEPAILLER", Color(0.19, 0.35, 0.74), Color(0.90, 0.94, 0.96))
    _box(board, "editorial timeline", Vector3(0, 0.015, 0.43), Vector3(0.88, 0.003, 0.01), COLORS.charcoal)
    for beat in BEATS:
        var x: float = -0.44 + 0.88 * float(beat.at) / LENGTH
        _box(board, "chapter tick", Vector3(x, 0.018, 0.43), Vector3(0.004, 0.004, 0.018), COLORS.cream)
    cursor = _sphere(board, "playback cursor", Vector3(-0.44, 0.025, 0.43), 0.009, COLORS.gold)
    _label("title", "FUJI 1976", Vector3(-0.51, 0.069, -0.36), 0.031, Color(1.0, 0.86, 0.60))
    _label("subtitle", "24 OCTOBER | CHAMPIONSHIP DECIDER", Vector3(-0.51, 0.047, -0.379), 0.014, Color.WHITE)
    _label("detail", "Lauda arrives three points ahead of Hunt.", Vector3(-0.51, 0.027, -0.402), 0.011, Color(0.84, 0.90, 0.92))
    _label("disclaimer", "ILLUSTRATIVE MODEL | NOT LAP ACCURATE", Vector3(0.56, 0.031, 0.39), 0.011, COLORS.cream, true)
    _label("state", "PLAY 1X", Vector3(0.55, 0.070, -0.36), 0.018, COLORS.gold, true)
    _label("chapter", "01 / 06", Vector3(0.55, 0.046, -0.385), 0.014, COLORS.gold, true)
    _label("race_facts", "73 LAPS  |  4.359 KM  |  FUJI", Vector3(0.55, 0.034, -0.412), 0.009, COLORS.cream, true)
    _add_button("PLAY", -0.50, Color(0.29, 0.63, 0.57), "pause")
    _add_button("BACK", -0.30, Color(0.76, 0.75, 0.73), "previous")
    _add_button("NEXT", -0.10, Color(0.94, 0.70, 0.24), "next")
    _add_button("SCALE", 0.10, Color(0.34, 0.53, 0.89), "scale")
    _add_button("SPEED", 0.30, Color(0.78, 0.36, 0.37), "speed")
    _add_button("CENTER", 0.50, Color(0.43, 0.70, 0.79), "center")
    var hint := _label("control_hint", "AIM + TRIGGER TO SELECT   |   B/Y TO RECENTER",
        Vector3(0.0, 0.130, 0.0), 0.014, COLORS.cream, false, ui_panel)
    hint.rotation.x = 0.0
    hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
    _update_story()
    _update_cars()

func _add_button(label_text: String, x: float, color: Color, action: String) -> void:
    _box(ui_panel, "Button backing " + label_text,
        Vector3(x, 0.012, -0.020), Vector3(0.164, 0.135, 0.012), COLORS.charcoal)
    var orb := _sphere(ui_panel, "Action " + label_text, Vector3(x, 0.040, 0.017), 0.034, color)
    var label3d := _label("button_" + label_text, label_text,
        Vector3(x, -0.032, 0.025), 0.012, Color.WHITE, false, ui_panel)
    label3d.rotation.x = 0.0
    label3d.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
    controls.append({"orb": orb, "action": action})

func _add_pointer(controller: XRController3D, side: String) -> void:
    # Never show a phantom pointer if tracking is lost.
    var beam := MeshInstance3D.new()
    beam.name = side + " controller pointer"
    var ray := CylinderMesh.new()
    ray.top_radius = 0.0015
    ray.bottom_radius = 0.0015
    ray.height = 1.6
    beam.mesh = ray
    var mat := _material(Color(0.98, 0.76, 0.33, 0.7))
    mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
    mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
    beam.material_override = mat
    beam.rotation.x = PI / 2.0
    beam.position.z = -0.8
    beam.visible = false
    controller.add_child(beam)
    beams[side] = beam

func _hit_control(controller: XRController3D) -> String:
    var origin: Vector3 = controller.global_position
    var direction: Vector3 = -controller.global_transform.basis.z.normalized()
    var nearest := 100.0
    var selected := ""
    for b in controls:
        var center: Vector3 = (b.orb as Node3D).global_position
        var along: float = (center - origin).dot(direction)
        if along < 0.025 or along > 2.0:
            continue
        var radius: float = 0.066
        if (center - (origin + direction * along)).length() < radius and along < nearest:
            nearest = along
            selected = b.action
    return selected

func _show_hover(action: String) -> void:
    if action == hovered_action:
        return
    hovered_action = action
    for b in controls:
        var orb := b.orb as MeshInstance3D
        orb.scale = Vector3.ONE * (1.19 if b.action == action else 1.0)
    if labels.has("control_hint"):
        labels["control_hint"].text = ("TRIGGER: " + action.to_upper() + "  |  B/Y: CENTER") if action != "" else "AIM + TRIGGER TO SELECT   |   B/Y TO RECENTER"

func _update_story() -> void:
    chapter = 0
    for i in range(1, BEATS.size()):
        if playback >= float(BEATS[i].at):
            chapter = i
    labels["title"].text = BEATS[chapter].title
    labels["subtitle"].text = BEATS[chapter].subtitle
    labels["detail"].text = BEATS[chapter].detail
    labels["chapter"].text = "%02d / %02d" % [chapter + 1, BEATS.size()]

func _position_car(key: String, offset: float, lane: float, rate: float, active: bool) -> void:
    var car: Node3D = car_nodes[key]
    car.visible = active
    if not active:
        return
    var t := playback * rate / 16.0 + offset
    var p: Vector3 = track.at(t) + track.right(t) * lane
    p.y = 0.037
    car.position = p
    var heading: Vector3 = track.tangent(t)
    car.rotation.y = atan2(-heading.x, -heading.z)

func _update_cars() -> void:
    _position_car("ANDRETTI", 0.00, 0.000, 1.10, true)
    _position_car("HUNT", 0.25, -0.012, 1.03, true)
    _position_car("LAUDA", 0.48, 0.010, 1.04, playback < 30.0)
    _position_car("DEPAILLER", 0.72, 0.012, 1.01, true)
    labels["state"].text = ("PLAY " if playing else "PAUSE ") + "%dX" % int(speed)
    cursor.position.x = -0.44 + 0.88 * playback / LENGTH

func _process(delta: float) -> void:
    intro_time += delta
    if playing:
        var previous := chapter
        playback = fposmod(playback + maxf(delta, 0.0) * speed, LENGTH)
        var new_chapter := 0
        for i in range(1, BEATS.size()):
            if playback >= float(BEATS[i].at):
                new_chapter = i
        if previous != new_chapter:
            _update_story()
    _update_cars()
    _read_controller()

func _read_controller() -> void:
    if not xr_active:
        return
    var current_hover := ""
    var action_taken := false
    # Independent rising-edge latches, at most one action each frame.
    for side in ["left", "right"]:
        var controller: XRController3D = left_controller if side == "left" else right_controller
        var tracked := controller.get_is_active() and controller.get_has_tracking_data()
        (beams[side] as MeshInstance3D).visible = tracked
        if not tracked:
            trigger_down[side] = false
            center_down[side] = false
            continue
        var hit := _hit_control(controller)
        if current_hover == "" and hit != "":
            current_hover = hit
        var pressed := controller.get_float("trigger") > 0.65 or controller.is_button_pressed("trigger_click")
        if pressed and not bool(trigger_down[side]) and hit != "":
            if not action_taken:
                _action(hit)
                action_taken = true
        trigger_down[side] = pressed
        var center_pressed := controller.is_button_pressed("by_button")
        if center_pressed and not bool(center_down[side]):
            if not action_taken:
                _action("center")
                action_taken = true
        center_down[side] = center_pressed
    _show_hover(current_hover)

func _action(action: String) -> void:
    match action:
        "pause": playing = not playing
        "next":
            chapter = (chapter + 1) % BEATS.size()
            playback = float(BEATS[chapter].at)
            _update_story()
        "previous":
            chapter = (chapter + BEATS.size() - 1) % BEATS.size()
            playback = float(BEATS[chapter].at)
            _update_story()
        "scale":
            expanded = not expanded
            board.scale = Vector3.ONE * (2.30 if expanded else tabletop_scale)
            _recenter()
        "speed": speed = 1.0 if speed >= 4.0 else speed * 2.0
        "center": _recenter()
    _update_cars()

func _recenter() -> void:
    var cam: Node3D = xr_camera if xr_active else desktop_camera
    var forward: Vector3 = -cam.global_transform.basis.z
    forward.y = 0.0
    if forward.length_squared() < 0.0001:
        forward = Vector3.FORWARD
    else:
        forward = forward.normalized()
    board.global_position = cam.global_position + forward * (2.05 if expanded else 0.91) - Vector3.UP * (0.68 if expanded else 0.40)
    # Board local +Z is its viewer-facing edge. Aim it back toward the headset;
    # the previous heading flipped the diorama 180 degrees at default forward.
    board.rotation.y = atan2(-forward.x, -forward.z)
    ui_panel.global_position = cam.global_position + forward * 0.68 - Vector3.UP * 0.31
    ui_panel.rotation.y = board.rotation.y

func _unhandled_key_input(event: InputEvent) -> void:
    if not event is InputEventKey:
        return
    var key_event := event as InputEventKey
    if not key_event.pressed or key_event.echo:
        return
    match key_event.keycode:
        KEY_SPACE: _action("pause")
        KEY_N: _action("next")
        KEY_P: _action("previous")
        KEY_M: _action("scale")
        KEY_F: _action("speed")
        KEY_R: _action("center")

func _exit_tree() -> void:
    if xr_interface != null and xr_interface.is_initialized():
        xr_interface.uninitialize()
