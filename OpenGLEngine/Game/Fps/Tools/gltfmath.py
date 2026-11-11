import json
import math
import struct


def load_glb(path):
    data = open(path, 'rb').read()
    json_length = struct.unpack('<I', data[12:16])[0]
    document = json.loads(data[20:20 + json_length])
    offset = 20 + json_length
    bin_length = struct.unpack('<I', data[offset:offset + 4])[0]
    return document, bytearray(data[offset + 8:offset + 8 + bin_length])


def save_glb(path, document, binary):
    while len(binary) % 4:
        binary.append(0)
    document['buffers'][0]['byteLength'] = len(binary)
    encoded = json.dumps(document, separators=(',', ':')).encode()
    while len(encoded) % 4:
        encoded += b' '
    total = 12 + 8 + len(encoded) + 8 + len(binary)
    with open(path, 'wb') as f:
        f.write(struct.pack('<III', 0x46546C67, 2, total))
        f.write(struct.pack('<II', len(encoded), 0x4E4F534A))
        f.write(encoded)
        f.write(struct.pack('<II', len(binary), 0x004E4942))
        f.write(bytes(binary))


COMPONENTS = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}


def read_accessor(document, binary, index):
    accessor = document['accessors'][index]
    view = document['bufferViews'][accessor['bufferView']]
    count = accessor['count']
    width = COMPONENTS[accessor['type']]
    if accessor['componentType'] != 5126:
        raise ValueError('only float accessors supported')
    start = view.get('byteOffset', 0) + accessor.get('byteOffset', 0)
    stride = view.get('byteStride', width * 4)
    values = []
    for i in range(count):
        values.append(struct.unpack_from('<' + 'f' * width, binary, start + i * stride))
    return values


def append_accessor(document, binary, values, type_name, with_bounds=False):
    while len(binary) % 4:
        binary.append(0)
    width = COMPONENTS[type_name]
    start = len(binary)
    for value in values:
        binary.extend(struct.pack('<' + 'f' * width, *value))
    document['bufferViews'].append({'buffer': 0, 'byteOffset': start, 'byteLength': len(binary) - start})
    accessor = {'bufferView': len(document['bufferViews']) - 1, 'componentType': 5126, 'count': len(values), 'type': type_name}
    if with_bounds:
        accessor['min'] = [min(v[k] for v in values) for k in range(width)]
        accessor['max'] = [max(v[k] for v in values) for k in range(width)]
    document['accessors'].append(accessor)
    return len(document['accessors']) - 1


def q_mul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)


def q_inv(q):
    x, y, z, w = q
    return (-x, -y, -z, w)


def q_norm(q):
    length = math.sqrt(sum(c * c for c in q)) or 1.0
    return tuple(c / length for c in q)


def q_rotate(q, v):
    p = q_mul(q_mul(q, (v[0], v[1], v[2], 0.0)), q_inv(q))
    return (p[0], p[1], p[2])


def q_slerp(a, b, t):
    dot = sum(x * y for x, y in zip(a, b))
    if dot < 0.0:
        b = tuple(-c for c in b)
        dot = -dot
    if dot > 0.9995:
        return q_norm(tuple(x + (y - x) * t for x, y in zip(a, b)))
    theta = math.acos(max(-1.0, min(1.0, dot)))
    s = math.sin(theta)
    wa = math.sin((1 - t) * theta) / s
    wb = math.sin(t * theta) / s
    return tuple(wa * x + wb * y for x, y in zip(a, b))


def v_add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def v_sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def v_scale(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def v_lerp(a, b, t):
    return tuple(x + (y - x) * t for x, y in zip(a, b))
