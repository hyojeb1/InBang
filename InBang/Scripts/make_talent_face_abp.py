import sys

import unreal

SOURCE_ABP = "/MetaHumanCharacter/Face/ABP_Face"
TARGET_ABP = "/Game/InBang/ABP_TalentFace"
PARENT_CLASS = "/Script/InBang.TalentFaceAnimInstance"


def log(message):
    unreal.log("MAKEABP " + message)


def main(target_skeleton_path):
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
        ["/MetaHumanCharacter/Face", "/Game"], True)
    assets = unreal.EditorAssetLibrary
    if assets.does_asset_exist(TARGET_ABP):
        abp = assets.load_asset(TARGET_ABP)
    else:
        abp = assets.duplicate_asset(SOURCE_ABP, TARGET_ABP)
    if abp is None:
        log("result=FAIL reason=duplicate source=" + SOURCE_ABP)
        return 1

    if target_skeleton_path:
        skeleton = unreal.load_asset(target_skeleton_path)
        if skeleton is None:
            log("result=FAIL reason=skeleton path=" + target_skeleton_path)
            return 1
        abp.set_editor_property("target_skeleton", skeleton)

    parent = unreal.load_class(None, PARENT_CLASS)
    unreal.BlueprintEditorLibrary.reparent_blueprint(abp, parent)
    unreal.BlueprintEditorLibrary.compile_blueprint(abp)
    assets.save_asset(TARGET_ABP, only_if_is_dirty=False)

    generated = unreal.load_object(None, TARGET_ABP + ".ABP_TalentFace_C")
    log("result={} asset={} parent={} skeleton={}".format(
        "PASS" if generated is not None else "FAIL",
        TARGET_ABP,
        parent.get_name() if parent else "none",
        abp.get_editor_property("target_skeleton").get_path_name()))
    return 0 if generated is not None else 1


main(sys.argv[1] if len(sys.argv) > 1 else "")
