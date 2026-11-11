import unreal

CHARACTER_FOLDER = "/Game/MetaHumans"
CHARACTER_NAME = "InBangTalent"
CHARACTER_PATH = CHARACTER_FOLDER + "/" + CHARACTER_NAME
STAGE_MAP = "/Game/Lvl_Stage"
TALENT_TAG = "Talent"


def log(message):
    unreal.log("MAKEMH " + message)


def registry():
    return unreal.AssetRegistryHelpers.get_asset_registry()


def load_or_create_character():
    if unreal.EditorAssetLibrary.does_asset_exist(CHARACTER_PATH):
        log("step=create result=SKIP reason=exists path=" + CHARACTER_PATH)
        return unreal.load_asset(CHARACTER_PATH)

    character = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name=CHARACTER_NAME,
        package_path=CHARACTER_FOLDER,
        asset_class=unreal.MetaHumanCharacter,
        factory=unreal.new_object(type=unreal.MetaHumanCharacterFactoryNew))
    if character is None:
        log("step=create result=FAIL")
        return None
    unreal.EditorAssetLibrary.save_loaded_asset(character, only_if_is_dirty=False)
    log("step=create result=PASS path=" + CHARACTER_PATH)
    return character


def rig_texture_and_build(character):
    subsystem = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
    if not subsystem.try_add_object_to_edit(character):
        log("step=edit result=FAIL reason=already-open")
        return False

    try:
        if not subsystem.can_build_meta_human(character=character):
            rig = unreal.MetaHumanCharacterAutoRiggingRequestParams()
            rig.blocking = True
            rig.report_progress = False
            rig.rig_type = unreal.MetaHumanRigType.JOINTS_AND_BLEND_SHAPES
            subsystem.request_auto_rigging(character=character, params=rig)
            log("step=autorig result=REQUESTED")

            if not character.has_high_resolution_textures:
                textures = unreal.MetaHumanCharacterTextureRequestParams()
                textures.blocking = True
                textures.report_progress = False
                subsystem.request_texture_sources(character=character, params=textures)
            log("step=textures result={}".format("PASS" if character.has_high_resolution_textures else "FAIL"))
            unreal.EditorAssetLibrary.save_loaded_asset(character, only_if_is_dirty=False)

        if not subsystem.can_build_meta_human(character=character):
            log("step=build result=FAIL reason=cannot-build")
            return False

        build = unreal.MetaHumanCharacterEditorBuildParameters()
        build.pipeline_type = unreal.MetaHumanDefaultPipelineType.OPTIMIZED
        build.pipeline_quality = unreal.MetaHumanQualityLevel.MEDIUM
        build.absolute_build_path = CHARACTER_FOLDER
        build.common_folder_path = CHARACTER_FOLDER + "/Common"
        build.enable_wardrobe_item_validation = False
        try:
            subsystem.build_meta_human(character=character, params=build)
        except RuntimeError as logged_errors:
            log("step=build logged-errors={}".format(str(logged_errors).count("RuntimeError") + 1))
        saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        log("step=build result=DONE saved={}".format(saved))
        return True
    finally:
        if subsystem.is_object_added_for_editing(character):
            subsystem.remove_object_to_edit(character)


def find_assembled_blueprint():
    registry().scan_paths_synchronous([CHARACTER_FOLDER], True)
    for data in registry().get_assets_by_path(CHARACTER_FOLDER, recursive=True):
        name = str(data.asset_name)
        if str(data.asset_class_path.asset_name) == "Blueprint" and name.startswith("BP_") and CHARACTER_NAME in name:
            return str(data.package_name)
    return None


def place_in_stage(blueprint_path):
    try:
        unreal.EditorLoadingAndSavingUtils.load_map(STAGE_MAP)
    except RuntimeError as logged_errors:
        log("step=load-map logged-errors={}".format(str(logged_errors).count("RuntimeError") + 1))
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if not world.get_path_name().startswith(STAGE_MAP + "."):
        log("step=place result=FAIL reason=map-not-loaded world=" + world.get_path_name())
        return False
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actors.get_all_level_actors():
        if actor.actor_has_tag(TALENT_TAG):
            actors.destroy_actor(actor)

    blueprint = unreal.load_asset(blueprint_path)
    actor = actors.spawn_actor_from_object(blueprint, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if actor is None:
        log("step=place result=FAIL blueprint=" + blueprint_path)
        return False
    actor.set_editor_property("tags", [unreal.Name(TALENT_TAG)])
    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, STAGE_MAP)
    components = ",".join(c.get_name() for c in actor.get_components_by_class(unreal.SkeletalMeshComponent))
    log("step=place result={} actor={} blueprint={} skeletalMeshes={}".format(
        "PASS" if saved else "FAIL", actor.get_name(), blueprint_path, components))
    return saved


def main():
    registry().scan_paths_synchronous(["/Game", "/MetaHumanCharacter"], True)
    character = load_or_create_character()
    if character is None:
        return

    blueprint_path = find_assembled_blueprint()
    if blueprint_path is None:
        if not rig_texture_and_build(character):
            return
        blueprint_path = find_assembled_blueprint()
    else:
        log("step=build result=SKIP reason=exists blueprint=" + blueprint_path)

    if blueprint_path is None:
        log("step=find-blueprint result=FAIL folder=" + CHARACTER_FOLDER)
        return
    place_in_stage(blueprint_path)


try:
    main()
finally:
    unreal.SystemLibrary.quit_editor()
