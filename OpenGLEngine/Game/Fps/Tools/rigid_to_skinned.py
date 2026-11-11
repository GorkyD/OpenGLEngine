import struct
import sys
from gltfmath import load_glb, save_glb


def append_raw(document, binary, data, component_type, type_name, count):
    while len(binary) % 4:
        binary.append(0)
    start = len(binary)
    binary.extend(data)
    document['bufferViews'].append({'buffer': 0, 'byteOffset': start, 'byteLength': len(data)})
    document['accessors'].append({'bufferView': len(document['bufferViews']) - 1, 'componentType': component_type, 'count': count, 'type': type_name})
    return len(document['accessors']) - 1


def main(source_path, output_path):
    document, binary = load_glb(source_path)
    nodes = document['nodes']

    joints = list(range(len(nodes)))
    joint_of_node = {node: index for index, node in enumerate(joints)}

    identity = struct.pack('<16f', 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
    inverse_bind = append_raw(document, binary, identity * len(joints), 5126, 'MAT4', len(joints))

    document.setdefault('skins', [])
    skin_index = len(document['skins'])
    document['skins'].append({'joints': joints, 'inverseBindMatrices': inverse_bind, 'name': 'rigid_parts'})

    converted_meshes = {}
    for node_index, node in enumerate(nodes):
        if 'mesh' not in node:
            continue

        mesh_index = node['mesh']
        if mesh_index in converted_meshes and converted_meshes[mesh_index] != node_index:
            new_mesh = dict(document['meshes'][mesh_index])
            new_mesh['primitives'] = [dict(p, attributes=dict(p['attributes'])) for p in new_mesh['primitives']]
            document['meshes'].append(new_mesh)
            mesh_index = len(document['meshes']) - 1
            node['mesh'] = mesh_index
        converted_meshes[mesh_index] = node_index

        joint = joint_of_node[node_index]
        for primitive in document['meshes'][mesh_index]['primitives']:
            count = document['accessors'][primitive['attributes']['POSITION']]['count']
            joints_data = struct.pack('<4H', joint, 0, 0, 0) * count
            weights_data = struct.pack('<4f', 1.0, 0.0, 0.0, 0.0) * count
            primitive['attributes']['JOINTS_0'] = append_raw(document, binary, joints_data, 5123, 'VEC4', count)
            primitive['attributes']['WEIGHTS_0'] = append_raw(document, binary, weights_data, 5126, 'VEC4', count)

        node['skin'] = skin_index

    scene = document['scenes'][document.get('scene', 0)]
    document['nodes'].append({'name': 'viewmodel_root', 'children': list(scene['nodes'])})
    scene['nodes'] = [len(document['nodes']) - 1]

    save_glb(output_path, document, binary)
    print('wrote', output_path, 'joints', len(joints), 'skinned nodes', len(converted_meshes))


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
