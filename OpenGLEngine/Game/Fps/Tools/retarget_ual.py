import sys
from gltfmath import (append_accessor, load_glb, q_inv, q_mul, q_norm, q_rotate, q_slerp, read_accessor, save_glb, v_add, v_lerp, v_scale, v_sub)

BONE_MAP = {
    'Hips': 'pelvis',
    'Spine': 'spine_03',
    'Head': 'Head',
    'LeftArm': 'upperarm_l',
    'LeftForeArm': 'lowerarm_l',
    'LeftHand': 'hand_l',
    'RightArm': 'upperarm_r',
    'RightForeArm': 'lowerarm_r',
    'RightHand': 'hand_r',
    'LeftUpLeg': 'thigh_l',
    'LeftLeg': 'calf_l',
    'LeftFoot': 'foot_l',
    'RightUpLeg': 'thigh_r',
    'RightLeg': 'calf_r',
    'RightFoot': 'foot_r',
}

CLIP_MAP = {
    'Idle_Loop': 'idle_alt',
    'Walk_Loop': 'walk_alt',
    'Jog_Fwd_Loop': 'run',
    'Sprint_Loop': 'sprint',
    'Crouch_Fwd_Loop': 'crouch_walk',
    'Crouch_Idle_Loop': 'crouch_idle',
    'Death01': 'death_fall',
    'Hit_Chest': 'hit_chest',
    'Hit_Head': 'hit_head',
    'Punch_Cross': 'attack_punch',
    'Punch_Jab': 'attack_jab',
    'Sword_Attack': 'attack_swing',
    'Push_Loop': 'push',
}

FRAME_RATE = 30.0


class Rig:
    def __init__(self, document):
        self.document = document
        self.nodes = document['nodes']
        self.parent = {}
        for index, node in enumerate(self.nodes):
            for child in node.get('children', []):
                self.parent[child] = index
        self.by_name = {node.get('name'): index for index, node in enumerate(self.nodes)}
        self.order = []
        visited = set()

        def visit(index):
            if index in visited:
                return
            if index in self.parent:
                visit(self.parent[index])
            visited.add(index)
            self.order.append(index)

        for index in range(len(self.nodes)):
            visit(index)

    def rest_local(self, index):
        node = self.nodes[index]
        return (tuple(node.get('translation', (0.0, 0.0, 0.0))), q_norm(tuple(node.get('rotation', (0.0, 0.0, 0.0, 1.0)))), node.get('scale', (1.0, 1.0, 1.0))[0])

    def globals_from(self, locals_by_index):
        result = {}
        for index in self.order:
            position, rotation, scale = locals_by_index[index]
            if index in self.parent:
                parent_position, parent_rotation, parent_scale = result[self.parent[index]]
                result[index] = (v_add(parent_position, q_rotate(parent_rotation, v_scale(position, parent_scale))), q_mul(parent_rotation, rotation), parent_scale * scale)
            else:
                result[index] = (position, rotation, scale)
        return result


def sample_track(times, values, t, is_rotation):
    if t <= times[0]:
        return values[0]
    if t >= times[-1]:
        return values[-1]
    low, high = 0, len(times) - 1
    while high - low > 1:
        mid = (low + high) // 2
        if times[mid] <= t:
            low = mid
        else:
            high = mid
    span = times[high] - times[low]
    alpha = (t - times[low]) / span if span > 0 else 0.0
    if is_rotation:
        return q_slerp(values[low], values[high], alpha)
    return v_lerp(values[low], values[high], alpha)


def read_clip(document, binary, animation):
    tracks = {}
    duration = 0.0
    for channel in animation['channels']:
        sampler = animation['samplers'][channel['sampler']]
        path = channel['target']['path']
        if path not in ('translation', 'rotation', 'scale'):
            continue
        times = [value[0] for value in read_accessor(document, binary, sampler['input'])]
        values = read_accessor(document, binary, sampler['output'])
        if sampler.get('interpolation', 'LINEAR') == 'CUBICSPLINE':
            values = values[1::3]
        tracks[(channel['target']['node'], path)] = (times, values)
        duration = max(duration, times[-1])
    return tracks, duration


def source_locals_at(rig, tracks, t):
    result = {}
    for index in range(len(rig.nodes)):
        position, rotation, scale = rig.rest_local(index)
        if (index, 'translation') in tracks:
            times, values = tracks[(index, 'translation')]
            position = sample_track(times, values, t, False)
        if (index, 'rotation') in tracks:
            times, values = tracks[(index, 'rotation')]
            rotation = q_norm(sample_track(times, values, t, True))
        if (index, 'scale') in tracks:
            times, values = tracks[(index, 'scale')]
            scale = sample_track(times, values, t, False)[0]
        result[index] = (position, rotation, scale)
    return result


def retarget_frame(source, target, source_rest, target_rest, source_now, hip_ratio):
    target_globals = {}
    target_locals = {}
    for index in target.order:
        name = target.nodes[index].get('name')
        rest_position, rest_rotation, rest_scale = target_rest[index]
        source_name = BONE_MAP.get(name)

        if source_name is not None and source_name in source.by_name:
            source_index = source.by_name[source_name]
            delta = q_mul(source_now[source_index][1], q_inv(source_rest[source_index][1]))
            global_rotation = q_norm(q_mul(delta, rest_rotation))
        else:
            parent_rotation = target_globals[target.parent[index]][1] if index in target.parent else (0.0, 0.0, 0.0, 1.0)
            global_rotation = q_mul(parent_rotation, target.rest_local(index)[1])

        local_position, _, local_scale = target.rest_local(index)
        if name == 'Hips':
            source_index = source.by_name['pelvis']
            offset = v_scale(v_sub(source_now[source_index][0], source_rest[source_index][0]), hip_ratio)
            world_position = v_add(rest_position, offset)
            if index in target.parent:
                parent_position, parent_rotation, parent_scale = target_globals[target.parent[index]]
                local_position = v_scale(q_rotate(q_inv(parent_rotation), v_sub(world_position, parent_position)), 1.0 / parent_scale)
            else:
                local_position = world_position

        if index in target.parent:
            parent_position, parent_rotation, parent_scale = target_globals[target.parent[index]]
            local_rotation = q_norm(q_mul(q_inv(parent_rotation), global_rotation))
            world_position = v_add(parent_position, q_rotate(parent_rotation, v_scale(local_position, parent_scale)))
        else:
            local_rotation = global_rotation
            world_position = local_position

        target_locals[index] = (local_position, local_rotation, local_scale)
        target_globals[index] = (world_position, global_rotation, rest_scale)
    return target_locals, target_globals


def main(target_path, source_path, output_path):
    target_doc, target_bin = load_glb(target_path)
    source_doc, source_bin = load_glb(source_path)
    target = Rig(target_doc)
    source = Rig(source_doc)

    target_rest = target.globals_from({i: target.rest_local(i) for i in range(len(target.nodes))})
    source_rest = source.globals_from({i: source.rest_local(i) for i in range(len(source.nodes))})

    hip_ratio = target_rest[target.by_name['Hips']][0][1] / source_rest[source.by_name['pelvis']][0][1]
    mapped_targets = [target.by_name[name] for name in BONE_MAP if name in target.by_name]
    hips_index = target.by_name['Hips']

    existing = {animation.get('name') for animation in target_doc.get('animations', [])}
    target_doc.setdefault('animations', [])

    for animation in source_doc.get('animations', []):
        clip_name = CLIP_MAP.get(animation.get('name'))
        if clip_name is None or clip_name in existing:
            continue

        tracks, duration = read_clip(source_doc, source_bin, animation)
        frame_count = max(2, int(round(duration * FRAME_RATE)) + 1)
        times = [min(duration, i / FRAME_RATE) for i in range(frame_count)]

        rotations = {index: [] for index in mapped_targets}
        hip_positions = []
        for t in times:
            source_now = source.globals_from(source_locals_at(source, tracks, t))
            target_locals, _ = retarget_frame(source, target, source_rest, target_rest, source_now, hip_ratio)
            for index in mapped_targets:
                rotation = target_locals[index][1]
                previous = rotations[index][-1] if rotations[index] else None
                if previous is not None and sum(a * b for a, b in zip(previous, rotation)) < 0.0:
                    rotation = tuple(-c for c in rotation)
                rotations[index].append(rotation)
            hip_positions.append(target_locals[hips_index][0])

        time_accessor = append_accessor(target_doc, target_bin, [(t,) for t in times], 'SCALAR', with_bounds=True)
        samplers = []
        channels = []
        for index in mapped_targets:
            samplers.append({'input': time_accessor, 'output': append_accessor(target_doc, target_bin, rotations[index], 'VEC4'), 'interpolation': 'LINEAR'})
            channels.append({'sampler': len(samplers) - 1, 'target': {'node': index, 'path': 'rotation'}})
        samplers.append({'input': time_accessor, 'output': append_accessor(target_doc, target_bin, hip_positions, 'VEC3'), 'interpolation': 'LINEAR'})
        channels.append({'sampler': len(samplers) - 1, 'target': {'node': hips_index, 'path': 'translation'}})

        target_doc['animations'].append({'name': clip_name, 'samplers': samplers, 'channels': channels})
        print(f'baked {animation.get("name"):18s} -> {clip_name:14s} {frame_count:4d} frames, {duration:.2f}s')

    save_glb(output_path, target_doc, target_bin)
    print('wrote', output_path)


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2], sys.argv[3])
