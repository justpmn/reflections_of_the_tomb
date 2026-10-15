#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct Point { int x, y; };
struct Color { unsigned char r, g, b; };
using Polygon = std::vector<Point>;
constexpr Color background{19,23,32}, panel{28,34,46}, ink{226,222,207};
constexpr Color stone{138,131,112}, dark{12,15,22}, red{191,25,56};

class Image {
    int width_, height_;
    std::vector<Color> pixels_;
public:
    Image(int w, int h, Color c) : width_(w), height_(h), pixels_(w*h,c) {}
    int width() const { return width_; }
    int height() const { return height_; }
    void setPixel(int x, int y, Color c) {
        if (x>=0 && x<width_ && y>=0 && y<height_) pixels_[y*width_+x]=c;
    }
    Color pixel(int x, int y) const { return pixels_.at(y*width_+x); }
    void save(const std::string& path) const {
        std::ofstream out(path,std::ios::binary);
        if (!out) throw std::runtime_error("Cannot open " + path);
        out << "P6\n" << width_ << ' ' << height_ << "\n255\n";
        for (Color c : pixels_) {
            const char bytes[]{static_cast<char>(c.r),static_cast<char>(c.g),static_cast<char>(c.b)};
            out.write(bytes,3);
        }
        if (!out) throw std::runtime_error("Failed writing " + path);
    }
};

// Integer Bresenham algorithm: all octants, both endpoints included.
void line(Image& im, Point a, Point b, Color c) {
    // Fix traversal order so tie cases have identical coverage in either direction.
    if (a.x>b.x || (a.x==b.x && a.y>b.y)) std::swap(a,b);
    int dx=std::abs(b.x-a.x), dy=-std::abs(b.y-a.y);
    int sx=a.x<b.x?1:-1, sy=a.y<b.y?1:-1, error=dx+dy;
    for (;;) {
        im.setPixel(a.x,a.y,c);
        if (a.x==b.x && a.y==b.y) break;
        int twice=2*error;
        if (twice>=dy) { error+=dy; a.x+=sx; }
        if (twice<=dx) { error+=dx; a.y+=sy; }
    }
}

// Even-odd scanline fill, sampling at pixel centers. Half-open edge intervals
// avoid counting shared vertices twice; horizontal edges do not intersect.
void fill(Image& im, const Polygon& p, Color c) {
    if (p.size()<3) return;
    int low=p[0].y, high=low;
    for (Point v:p) { low=std::min(low,v.y); high=std::max(high,v.y); }
    for (int y=std::max(0,low); y<std::min(im.height(),high); ++y) {
        double scan=y+0.5;
        std::vector<double> xs;
        for (std::size_t i=0; i<p.size(); ++i) {
            Point a=p[i], b=p[(i+1)%p.size()];
            if ((a.y<=scan && b.y>scan) || (b.y<=scan && a.y>scan))
                xs.push_back(a.x+(scan-a.y)*(b.x-a.x)/(b.y-a.y));
        }
        std::sort(xs.begin(),xs.end());
        for (std::size_t i=0; i+1<xs.size(); i+=2) {
            int first=std::max(0,static_cast<int>(std::ceil(xs[i]-0.5)));
            int end=std::min(im.width(),static_cast<int>(std::ceil(xs[i+1]-0.5)));
            for (int x=first; x<end; ++x) im.setPixel(x,y,c);
        }
    }
}
void polygon(Image& im, const Polygon& p, Color c, Color edge=ink) {
    fill(im,p,c);
    if (p.size()<2) return;
    for (std::size_t i=0; i<p.size(); ++i) line(im,p[i],p[(i+1)%p.size()],edge);
}
Polygon rectangle(int x,int y,int w,int h) {
    return {{x,y},{x+w,y},{x+w,y+h},{x,y+h}};
}
Polygon translate(const Polygon& p,int dx,int dy) {
    Polygon result;
    for (Point v:p) result.push_back({v.x+dx,v.y+dy});
    return result;
}

// Small 5x7 bitmap alphabet; labels also use our setPixel function.
void label(Image& im,int x,int y,const std::string& text,Color c,int scale=2) {
    static const char* glyphs[]={
        "01110100011000111111100011000110001", // A
        "11110100011000111110100011000111110", // B
        "01111100001000010000100001000001111", // C
        "11110100011000110001100011000111110", // D
        "11111100001000011110100001000011111", // E
        "11111100001000011110100001000010000", // F
        "01111100001000010111100011000101111", // G
        "10001100011000111111100011000110001", // H
        "11111001000010000100001000010011111", // I
        "00111000100001000010100101001001100", // J
        "10001100101010011000101001001010001", // K
        "10000100001000010000100001000011111", // L
        "10001110111010110101100011000110001", // M
        "10001110011010110011100011000110001", // N
        "01110100011000110001100011000101110", // O
        "11110100011000111110100001000010000", // P
        "01110100011000110001101011001001101", // Q
        "11110100011000111110101001001010001", // R
        "01111100001000001110000010000111110", // S
        "11111001000010000100001000010000100", // T
        "10001100011000110001100011000101110", // U
        "10001100011000110001100010101000100", // V
        "10001100011000110101101011010101010", // W
        "10001100010101000100010101000110001", // X
        "10001100010101000100001000010000100", // Y
        "11111000010001000100010001000011111"  // Z
    };
    for (char ch:text) {
        if (ch>='A' && ch<='Z')
            for (int row=0; row<7; ++row) for (int col=0; col<5; ++col)
                if (glyphs[ch-'A'][row*5+col]=='1')
                    for (int dy=0; dy<scale; ++dy) for (int dx=0; dx<scale; ++dx)
                        im.setPixel(x+col*scale+dx,y+row*scale+dy,c);
        x+=6*scale;
    }
}

void drawRuby(Image& im) {
    // Task 1: explicit (x,y) boundary vertices with red polygon filling.
    Polygon ruby{{210,125},{300,125},{345,170},{255,260},{165,170}};
    polygon(im,ruby,red);
    polygon(im,{{210,125},{300,125},{282,170},{228,170}},{247,73,100});
    polygon(im,{{165,170},{228,170},{255,260}},{133,13,44});
    polygon(im,{{228,170},{282,170},{255,260}},{224,36,69});
    line(im,{282,170},{345,170},ink);
    label(im,410,143,"RUBY",ink,3);
    label(im,410,185,"POLYGON VERTICES",stone);
    label(im,410,215,"RED SCANLINE FILL",stone);
}

void drawDoor(Image& im,int originX,bool open) {
    // Task 2: closed door (dx,dy)=(0,0); open door (0,-100).
    int x=originX+60,y=391,dy=open?-100:0;
    polygon(im,rectangle(x-16,y-18,152,167),stone);
    polygon(im,rectangle(x,y,120,145),dark,dark);
    Polygon door=rectangle(x,y,120,145);
    polygon(im,translate(door,0,dy),{98,72,47},{196,157,99});
    for (int offset:{24,48,72,96})
        line(im,{x+offset,y+dy+1},{x+offset,y+dy+144},{65,46,32});
    for (int offset:{26,112})
        polygon(im,translate(rectangle(x+8,y+offset,104,8),0,dy),stone,stone);
    polygon(im,translate(rectangle(x+96,y+72,8,8),0,dy),ink);
    line(im,{originX+24,540},{originX+420,540},stone);
    label(im,originX+225,410,open?"OPEN":"CLOSED",ink);
    label(im,originX+225,445,open?"MOVED UP":"AT REST",stone);
    if (open) {
        line(im,{originX+390,393},{originX+390,310},ink);
        line(im,{originX+390,310},{originX+383,321},ink);
        line(im,{originX+390,310},{originX+397,321},ink);
    }
}

void drawSpikes(Image& im,int originX,bool activated) {
    // Task 3: repeated triangle polygons, retracted downward when deactivated.
    int floor=738,dy=activated?0:50;
    for (int i=0;i<5;++i) {
        int x=originX+40+i*68;
        Polygon spike{{x,floor},{x+27,floor-60},{x+54,floor}};
        polygon(im,translate(spike,0,dy),{169,182,193});
        line(im,{x+27,floor-60+dy},{x+27,floor+dy},{92,111,130});
    }
    // Foreground floor occludes the retracted portions of the triangles.
    fill(im,rectangle(originX+28,floor+1,360,55),panel);
    line(im,{originX+28,floor},{originX+388,floor},stone);
    label(im,originX+40,637,activated?"ACTIVATED":"DEACTIVATED",ink);
    label(im,originX+40,769,activated?"SPIKES RAISED":"SPIKES RETRACTED",stone);
}

bool same(Color a,Color b) { return a.r==b.r && a.g==b.g && a.b==b.b; }
void selfTest() {
    auto check=[](bool ok) { if (!ok) throw std::runtime_error("Self-test failed"); };
    for (Point end:Polygon{{9,7},{7,9},{3,9},{1,7},{1,3},{3,1},{7,1},{9,3},{5,5}}) {
        Image a(11,11,dark),b(11,11,dark);
        line(a,{5,5},end,red); line(b,end,{5,5},red);
        check(same(a.pixel(end.x,end.y),red));
        for (int y=0;y<11;++y) for (int x=0;x<11;++x)
            check(same(a.pixel(x,y),b.pixel(x,y)));
    }
    Image im(12,12,dark);
    fill(im,{{1,1},{10,1},{10,4},{4,4},{4,10},{1,10}},red);
    check(same(im.pixel(2,8),red) && same(im.pixel(8,2),red));
    check(same(im.pixel(8,8),dark)); // concave notch
    fill(im,rectangle(-5,-5,7,7),ink);
    check(same(im.pixel(0,0),ink));
    Polygon moved=translate({{1,2},{3,4},{5,6}},7,-10);
    check(moved[0].x==8 && moved[0].y==-8 && moved[2].y==-4);
    std::cout << "Passed: line octants, concave filling, clipping, translation.\n";
}

int main(int argc,char** argv) {
    try {
        if (argc>1 && std::string(argv[1])=="--self-test") { selfTest(); return 0; }
        std::string path=argc>1?argv[1]:"tomb_graphics.ppm";
        Image im(1000,830,background);
        label(im,32,24,"REFLECTIONS OF THE TOMB",ink,3);
        for (Polygon box:{rectangle(24,78,952,204),rectangle(24,294,952,268),rectangle(24,574,952,232)})
            fill(im,box,panel);
        label(im,42,92,"RUBY",stone); drawRuby(im);
        label(im,42,308,"TEMPLE DOOR",stone);
        drawDoor(im,40,false); drawDoor(im,530,true);
        label(im,42,590,"SPIKE TRAP",stone);
        drawSpikes(im,60,true); drawSpikes(im,550,false);
        im.save(path);
        std::cout << "Saved " << path << " (1000 x 830 binary PPM).\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
