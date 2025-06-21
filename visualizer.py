import numpy as np
from matplotlib import pyplot as plt
from matplotlib.lines import Line2D
import math
import random
from itertools import islice


PERP = np.array([[0., -1.], [1., 0.]])
RPERP = np.array([[0., 1.], [-1., 0.]])
epsilon = 1e-6

def window(seq, n=2):
    it = iter(seq)
    result = tuple(islice(it, n))
    if len(result) == n:
        yield result
    for elem in it:
        result = result[1:] + (elem,)
        yield result

def offset_point(prev, curr, next, distance):
    if prev is None and next is None:
        raise ValueError("Fuck you.")
    if prev is None:
        d = next - curr
        d /= np.linalg.norm(d)
        v = PERP @ (curr - next)
        return (curr - d*abs(distance)) + v/np.linalg.norm(v) * distance
    if next is None:
        v = RPERP @ (curr - prev)
        d = curr - prev
        d /= np.linalg.norm(d)
        return (curr + d*abs(distance)) + v/np.linalg.norm(v) * distance

    n1 = RPERP @ (curr - prev)
    n2 = PERP @ (curr - next)
    n1 /= np.linalg.norm(n1)
    n2 /= np.linalg.norm(n2)

    length = distance/np.sqrt(0.5 + 0.5*(n1 @ n2))
    
    bisector = n1 + n2
    bisector /= np.linalg.norm(bisector)
    return curr + bisector*length

def cross2d(v1, v2):
    return v1[..., 0] * v2[..., 1] - v1[..., 1] * v2[..., 0]

def get_line_segment_intersection(seg1_p1, seg1_p2, seg2_p1, seg2_p2):
    if seg1_p1[0] == seg1_p2[0] and seg1_p1[1] == seg1_p2[1]:
        return None
    if seg2_p1[0] == seg2_p2[0] and seg2_p1[1] == seg2_p2[1]:
        return None
    seg1_p1 = np.array(seg1_p1)
    seg1_p2 = np.array(seg1_p2)
    seg2_p1 = np.array(seg2_p1)
    seg2_p2 = np.array(seg2_p2)
    
    seg1_dir = seg1_p2 - seg1_p1
    seg2_dir = seg2_p2 - seg2_p1

    cross = cross2d(seg1_dir, seg2_dir)
    if abs(cross) < epsilon:
        # they're parallel, i don't care about the intersection if they
        # happen to overlap, i don't think this will ever happen
        return None

    to_seg2 = seg2_p1 - seg1_p1
    t1 = cross2d(to_seg2, seg2_dir) / cross
    t2 = cross2d(to_seg2, seg1_dir) / cross
    if 0 <= t1 <= 1 and 0 <= t2 <= 1:
        intersection = seg1_p1 + t1 * seg1_dir
        return intersection
    else:
        return None


def create_offset(trail_path, offset):
    points = []
    labels = []
    for p, c, n in window([None, *trail_path, None], 3):
        x = offset_point(p, c, n, offset)
        points.append(x)
        if p is None:
            continue
        actual_direction = c - p
        actual_direction /= np.linalg.norm(actual_direction)
        trail_direction = points[-1] - points[-2]
        if trail_direction[0] == 0 and trail_direction[1] == 0:
            labels.append(True)
            continue
        trail_direction /= np.linalg.norm(trail_direction)
        if actual_direction @ trail_direction < -1 + epsilon:
            labels.append(False)
        else:
            labels.append(True)
    return points, labels


def fix_points(trail_path, offset, points, labels, other_points):
    for i in range(len(labels)):
        if labels[i]:
            continue
        # if there are 2 invalids in a row, select the shorter one to fix,
        # this helps fix sharp angles at endpoints sometimes
        if i+1 != len(labels) and not labels[i+1] and (
            np.linalg.norm(trail_path[i+1] - trail_path[i]) >
            np.linalg.norm(trail_path[i+2] - trail_path[i+1])
        ):
            continue
        # the line before the broken trail line, looking for intersections
        # forwards
        next_segment = (points[0], other_points[0]) if i == 0 else (
            list(points[i-1]),
            list(offset_point(trail_path[i-1], trail_path[i], None, offset))
        )
        # the line after the broken trail line, looking for intersections
        # backwards
        prev_segment = (
            (points[-1], other_points[-1])
            if i+2 >= len(points) else (
                list(points[i+2]),
                list(offset_point(
                    None, trail_path[i+1], trail_path[i+2], offset
                ))
            )
        )
        next_finished = False
        prev_finished = False
        for j in range(2, len(points)+1):
            if i+j >= len(points) and not next_finished:
                next_finished = True
                intersection = get_line_segment_intersection(
                    list(points[-1]), list(other_points[-1]), *next_segment
                )
                if intersection is not None:
                    points[i:i+j] = j*[intersection]
                    labels[i:i+j-1] = (j-1)*[True]
                    break
            if i+1-j < 0:
                prev_finished = True
                intersection = get_line_segment_intersection(
                    list(points[0]), list(other_points[0]), *prev_segment
                )
                if intersection is not None:
                    points[i+2-j:i+2] = j*[intersection]
                    labels[i+2-j:i+2] = j*[True]
                    break
            if not next_finished:
                intersection = get_line_segment_intersection(
                    points[i-1+j], points[i+j], *next_segment
                )
                if intersection is not None:
                    points[i:i+j] = j*[intersection]
                    labels[i:i+j-1] = (j-1)*[True]
                    break
            if not prev_finished:
                intersection = get_line_segment_intersection(
                    points[i+1-j], points[i+2-j], *prev_segment
                )
                if intersection is not None:
                    points[i+2-j:i+2] = j*[intersection]
                    labels[i+2-j:i+2] = j*[True]
                    break
            if next_finished and prev_finished:
                # in the actual mod, this will just be a break, not a throw
                # something terrible has happened, and i can't fix it
                raise ValueError

def draw_trail(ax, offset1, offset2, color, triangulate=True):
    if not triangulate:
        ax.fill(
            [
                *np.array(offset1).T[0],
                *np.array(list(reversed(offset2))).T[0],
                offset1[0][0]
            ],
            [
                *np.array(offset1).T[1],
                *np.array(list(reversed(offset2))).T[1],
                offset1[0][1]
            ],
            color=color
        )
        return

    for ((p1, p4), (p2, p3)) in window(zip(offset1, offset2), 2):
        ax.fill([p1[0], p2[0], p3[0]], [p1[1], p2[1], p3[1]], color=color)
        ax.fill([p3[0], p4[0], p1[0]], [p3[1], p4[1], p1[1]], color=color)


paths = [
    np.array([[0., 0.], [20., 20.]]),
    np.array([[0., 0.], [0.1, 0.1]]),
    np.array([
        [0, 10],
        [5, 5],
        [15, 5],
        [17, 3],
        [17.01, 5],
        [19.01, 3],
        [19.02, 5],
        [21.02, 3],
        [21.03, 5],
        [23.03, 3],
        [23.04, 5],
        [25.04, 3],
        [25.05, 5],
        [60.05, 10],
    ]),
    np.array([
        [21., 11],
        [11., 2],
        [9, 2],
        [9, 0],
        [8, 0],
        [8, -2],
        [-0, 5],
        [-5, -10],
    ]),
    np.array([
        [0., 10],
        [10, 0],
        [10, 1],
        [13, 1],
        [22, 12],
    ]),
    np.array([
        [0, 10],
        [10, 0],
        [10.01, 0],
        [10.01, 0.01],
        [10.02, 0.01],
        [10.02, 0.02],
        [10.03, 0.02],
        [10.03, 0.03],
        [10.04, 0.03],
        [10.04, 0.04],
        [21, 11],
    ]),
    np.array([
        [-1, 1],
        [0, 0],
        [1, 1],
        [4, -2.],
        [8, 2.],
    ]),
    np.array([
        [0, 0],
        [1, 100],
        [3, -100.],
        [5, 100.],
        [7, -100.],
        [8, 0.],
        [9, -100.],
        [11, 100.],
        [15, 104]
    ][::-1]),
    np.array([
        [0, 0],
        [0.125*1.0, 100],
        [0.125*1.5, 50.],
        [0.125*2.0, 100.],
        [50, 148]
    ][::1]),
    np.array([
        [0, 0],
        [0.5, 0.25],
        [1, 0],
        [2.5, .5],
    ][::1]),
    np.array([
        [0, 0],
        [0.1, 0.1],
        [5, -3.],
    ]),
    # this one will not always work, probably because of self-intersecting
    # paths, which are not supported, as the trail path should not be able to
    # intersect itself in the actual game; i hope that every other path works
    # well though, even if not perfect
    # some self-intersecting paths work fine, others do not, not sure what the
    # condition for them to break is
    # feel free to pr a fix for self-intersecting paths (or tell me what
    # the break condition is if you happen to figure it out)
    # if you want to do that for some reason
    np.array([
        [random.uniform(0, 1000), random.uniform(0, 1000)] for _ in range(100)
    ])
]

def main():
    plt.style.use('dark_background')
    for trail_path in paths:
        fig, ax = plt.subplots()
        ax.set_aspect('equal')
        width = 2*3.8424535
        offset_pos, offset_pos_labels = create_offset(trail_path, width/2)
        offset_neg, offset_neg_labels = create_offset(trail_path, -width/2)
        fix_points(
            trail_path, width/2, offset_pos, offset_pos_labels, offset_neg
        )
        fix_points(
            trail_path, -width/2, offset_neg, offset_neg_labels, offset_pos
        )

        minimum = min([
            *np.array(offset_pos).T[0],
            *np.array(offset_pos).T[1],
            *np.array(offset_neg).T[0],
            *np.array(offset_neg).T[1],
        ])
        maximum = max([
            *np.array(offset_pos).T[0],
            *np.array(offset_pos).T[1],
            *np.array(offset_neg).T[0],
            *np.array(offset_neg).T[1],
        ])

        draw_trail(ax, offset_pos, offset_neg, '#8000ff8f', triangulate=True)

        width /= 3
        offset_pos, offset_pos_labels = create_offset(trail_path, width/2)
        offset_neg, offset_neg_labels = create_offset(trail_path, -width/2)
        fix_points(
            trail_path, width/2, offset_pos, offset_pos_labels, offset_neg
        )
        fix_points(
            trail_path, -width/2, offset_neg, offset_neg_labels, offset_pos
        )
        draw_trail(
            ax, offset_pos, offset_neg, f'#ffffff{hex(int(0x8f * 0.65))[2:]}',
            triangulate=True
        )

        ax.set_xlim(minimum, maximum)
        ax.set_ylim(minimum, maximum)

        ax.plot(
            *trail_path.T, solid_joinstyle='miter',
            solid_capstyle='butt', color='#ffffffff'
        )
        plt.show()
        ax.clear()

if __name__ == '__main__':
    main()
