#include <fstream>
#include <string>
#include <vector>
#include "base64.hpp"

#define STB_IMAGE_IMPLEMENTATION

#include "lib/stb_image.h"
// #include "lib/stb_image_write.h"

struct TileInfo {
  int tile;
  int vFlip;
  int hFlip;
  int priority;
  int meta;
};

struct TileSet {
    int width = 0;
    int height = 0;
    std::string encodedData;
};

TileInfo decode_word(uint16_t word) {
  int tile = word & 0xFF;
  int attr = (word >> 8) & 0xFF;
  int vFlip = attr & 0x01;
  int hFlip = (attr >> 1) & 0x01;
  int priority = (attr >> 4) & 0x01;
  int meta = (attr >> 5) & 0x07;

  return {tile, vFlip, hFlip, priority, meta};
}

// Reads a file as binary and returns its contents as a vector
std::vector<unsigned char> readFileBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return std::vector<unsigned char>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

// Returns base64-encoded PNG file data
std::string base64EncodePngFile(const std::string& path) {
    std::vector<unsigned char> fileData = readFileBinary(path);
    if (fileData.empty()) {
        std::cerr << "Failed to read image file: " << path << std::endl;
        return "";
    }
    return base64_encode(fileData.data(), fileData.size());
}

TileSet loadImage(const std::string& path) {
    TileSet tileSet;
    int width = 0, height = 0, channels = 0;

    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load image: " << path << std::endl;
    } else {
        tileSet.width = width;
        tileSet.height = height;
        tileSet.encodedData = base64EncodePngFile(path);
        stbi_image_free(data);
    }
    
    return tileSet;
}

int getImageWidth(const std::string& path) {
    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (data) stbi_image_free(data);
    return width;
}

void saveMetatileDocHtml(
    const std::vector<std::array<uint16_t, 4>>& metatiles,
    const std::string& tilesheet_path,
    const std::string& out_html_path,
    int tile_width = 8, 
    int tile_height = 8
) {
    TileSet tileSet = loadImage(tilesheet_path);
    int width = tileSet.width / tile_width;
    std::ofstream ofs(out_html_path);
        
    ofs << R"HTML(
    <!DOCTYPE html>
    <html>
    <head>
        <meta charset='utf-8' />
        <title>Metatiles</title>
        <style>
            .tilesheet { 
                background-image: url('data:image/png;base64,)HTML" << tileSet.encodedData << R"HTML('); 
                background-repeat: no-repeat; 
                image-rendering: pixelated;
            }
            .fullTilesheet { 
                background-repeat: no-repeat; 
                image-rendering: pixelated;
                display: block;
                width: )HTML" << tileSet.width << R"HTML(px;
                height: )HTML" << tileSet.height << R"HTML(px;
            }
            .meta {
                display: inline-block; 
                margin: 8px; 
                font-family: monospace;
                vertical-align: top;
            }
            .metaid { 
                font-size: 12px; 
                text-align: center; 
                background: #eee; 
            }
            .tile { 
                width:)HTML" << tile_width << R"HTML(px; 
                height:)HTML" << tile_height << R"HTML(px; 
                display: inline-block; 
            }
            .priority { }
            .meta-info { 
                font-size: 10px; 
                text-align: center; 
            }
            .overlay { 
                display: block; 
                width: 16px; 
                height: 16px; 
                color: #fff; 
                font-size: 9px; 
                text-align: center; 
                line-height: 1.1; 
                pointer-events: none; 
            }
            .scaleup { 
                transform: scale(6); 
                transform-origin: top left; 
            }
        </style>
    </head>
    <body>
    <h1>Metatiles</h1>
    <h2>Metatile Definitions</h2>
    <div>
    )HTML";

    for (size_t idx = 0; idx < metatiles.size(); ++idx) {
        ofs << "<div class='meta'>";
        ofs << "<div class='metaid'>ID " << (idx + 1) << "</div>";
        ofs << "<div style='clear: both; display: block; height: 128px; '><div class='scaleup' style='display: grid; grid-template-columns: repeat(2, " << tile_width << "px); grid-template-rows: repeat(2, " << tile_height << "px);'>";
        for (int i = 0; i < 4; ++i) {
            TileInfo info = decode_word(metatiles[idx][i]);
            std::string prio_class = info.priority ? "priority" : "";
            
            int tx = info.tile % width;
            int ty = info.tile / width;

            std::string flip;
            if (info.hFlip && info.vFlip)
                flip = "transform: scale(-1,-1);";
            else if (info.hFlip)
                flip = "transform: scale(-1,1);";
            else if (info.vFlip)
                flip = "transform: scale(1,-1);";
            std::string style =
                "background-position: " + std::to_string(-tx * tile_width) + "px " +
                std::to_string(-ty * tile_height) + "px; " + flip;
            ofs << "<div class='tile tilesheet " << prio_class << "' style='" << style << "'>";
            ofs << "<div class='overlay'>&nbsp;</div>";
            ofs << "</div>";
        }
        ofs << "</div></div>"; // close scaleup

        // Move meta-info OUTSIDE the scaleup div
        ofs << "<div class='meta-info'>";

        ofs << "Tile: ";
        for (int i = 0; i < 4; ++i) {
            ofs << decode_word(metatiles[idx][i]).tile;
            if (i != 3) ofs << ", ";
        }

        ofs << "<br/>Meta: ";
        for (int i = 0; i < 4; ++i) {
            ofs << decode_word(metatiles[idx][i]).meta;
            if (i != 3) ofs << ", ";
        }
        ofs << "<br/>Priority: ";
        for (int i = 0; i < 4; ++i) {
            ofs << decode_word(metatiles[idx][i]).priority;
            if (i != 3) ofs << ", ";
        }

        ofs << "<br/>H Flip: ";
        for (int i = 0; i < 4; ++i) {
            ofs << decode_word(metatiles[idx][i]).hFlip;
            if (i != 3) ofs << ", ";
        }
        
        ofs << "<br/>V Flip: ";
        for (int i = 0; i < 4; ++i) {
            ofs << decode_word(metatiles[idx][i]).vFlip;
            if (i != 3) ofs << ", ";
        }

        ofs << "</div>"; // close meta-info
        ofs << "</div>\n"; // close meta
    }
    
    ofs << R"HTML(
    <br style='clear: both;' />
    <hr />
    <h2>Tileset</h2>
    <div class='scaleup'>
        <div class="tilesheet fullTilesheet">&nbsp;</div>
    </div>
    </body>
    </html>
    )HTML";
    ofs.close();
}
