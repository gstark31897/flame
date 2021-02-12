import math
import random

from PIL import Image, ImageDraw
import colorsys

def v0(x, y, *arg):
    return x, y

def v1(x, y, *arg):
    return math.sin(x), math.sin(y)

def v2(x, y, r, *arg):
    c = 1/(r*r)
    return c * x, c * y

def v3(x, y, r, *arg):
    r2 = r*r
    return x*math.sin(r2) - y*math.cos(r2), x*math.cos(r2) + y*math.sin(r2)

def v13(x, y, r, omega, theta, *arg):
    sqr = math.sqrt(r)
    return sqr * math.cos(theta/2 + omega), sqr * math.sin(theta/2 + omega)

def v18(x, y, *arg):
    e = pow(math.e, x-1)
    return e*math.cos(math.pi*y), e*math.sin(math.pi*y)

def v19(x, y, r, omega, theta, *arg):
    r2 = pow(r, math.sin(theta))
    return r2*math.cos(theta), r2*math.sin(theta)

offset = random.random()
def make_affine(i, lim):
    affine = [random.random() * 2.0 - 1 for i in range(7)]
    affine[-1] = (1/lim*i + offset)%1
    return affine

functions = [v0, v1, v2, v3, v13, v19]
affine_count = 5
affines = [make_affine(i, affine_count) for i in range(affine_count)]
print(affines)
width, height = 900, 900
points = [(random.random() * 2 + 1, random.random() * 2 + 1, 0) for i in range(100000)]
output = [ [ [0,0] for y in range(height) ] for x in range(width) ]
falloff = [0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5,
    6, 6, 6, 7, 7, 8, 8, 8, 9, 10, 10, 10, 11, 12, 13, 14, 14,
    15, 16, 17, 18, 20, 21, 22, 24, 26, 27, 28, 30, 31, 32, 34,
    35, 36, 38, 39, 40, 41, 42, 44, 45, 46, 48, 49, 50, 52, 54,
    56, 58, 59, 61, 63, 65, 67, 69, 71, 72, 74, 76, 78, 80, 82,
    85, 88, 90, 92, 95, 98, 100, 103, 106, 109, 112, 116, 119,
    122, 125, 129, 134, 138, 142, 147, 151, 156, 160, 165, 170,
    175, 180, 185, 190, 195, 200, 207, 214, 221, 228, 234, 241,
    248, 255
]
highest = 0

def affine(x, y, z, a, b, c, d, e, f, g, i):
    #if z > 0:
    #    return a*x + b*y + c, d*x + e*y + f, z
    #return a*x + b*y + c, d*x + e*y + f, z+(g-z)/i
    return a*x + b*y + c, d*x + e*y + f, z + (g - z)*i/100

def affect(x, y):
    return random.choice(functions)(x, y, math.sqrt(x*x + y*y), random.random()*math.pi, math.atan(x/y))

def allign(x, scale):
    x = (x + 1)/2
    return int(x * scale)

def color(x, y, z):
    global highest

    if abs(x) >= 1 or abs(y) >= 1:
        return

    ax = int((x + 1)/2 * width)
    ay = int((y + 1)/2 * height)
    output[ax][ay][0] += 1
    output[ax][ay][1] += z
    output[ax][ay][1] /= 2
    if output[ax][ay][0] > highest:
        highest += 1

def move(p, i):
    global highest
    x, y, z = points[p]
    x, y, z = affine(x, y, z, *random.choice(affines), i)
    x, y = affect(x, y)
    if i > 20:
        color(x, y, z)
    points[i] = (x, y, z)

def update(i):
    for p in range(len(points)):
        move(p, i)

def draw(i):
    print(highest)
    img = Image.new('RGBA', (width, height), (0,0,0,0))
    pix = img.load()

    for x in range(height):
        for y in range(width):
            v = math.log(max(output[x][y][0] * output[x][y][0], 1), max(highest*highest, 2))
            s = 1 - v
            val = (int(i*255) for i in colorsys.hsv_to_rgb(output[x][y][1], s, v))
            pix[x, y] = (*val, 255)

    #img.show()
    img.save('{}.png'.format(i))

for i in range(1000):
    update(i)
draw(0)

