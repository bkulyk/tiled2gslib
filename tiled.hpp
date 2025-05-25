#include <vector>

#include "lib/tileson.hpp"

constexpr uint32_t FLIP_MASK = 0xE0000000;

// --- getTileData Function (Revised to return a single combined word) ---
// Extracts and encodes data for a single 8x8 tile into a single 16-bit combined word.
// This word integrates both the tile ID and its attributes.
uint16_t getTileData(int x, int y, tson::Layer* tileLayer, tson::Layer* priorityLayer, tson::Layer* metaLayer) {
  tson::Tile* tile = tileLayer->getTileData(x, y);
  tson::Tile* priorityTile = priorityLayer != nullptr ? priorityLayer->getTileData(x, y) : nullptr;
  tson::Tile* metaTile = metaLayer != nullptr ? metaLayer->getTileData(x, y) : nullptr;

  uint32_t raw_gid_32bit = 0;

  // --- 1. Extract Base Tile ID (0-based) ---
  uint16_t base_tile_id_0based = 0;
  if (tile != nullptr) {
    raw_gid_32bit = tile->getGid();

    base_tile_id_0based = (raw_gid_32bit & ~FLIP_MASK) - 1;
  }

  // --- 2. Extract Flip Flags ---
  int hFlip = tile->hasFlipFlags(tson::TileFlipFlags::Horizontally);
  int vFlip = tile->hasFlipFlags(tson::TileFlipFlags::Vertically);

  // --- 3. Determine Priority and Meta ID ---
  int priority_flag = (priorityTile != nullptr && priorityTile->getGid() > 0) ? 1 : 0;
  uint16_t current_meta_id = 0;
  if (metaTile != nullptr && metaTile->getGid() > 0) {
    if (metaTile->getTileset() != nullptr) {
      current_meta_id = static_cast<uint16_t>(metaTile->getGid() - metaTile->getTileset()->getFirstgid() + 1);
      if (current_meta_id > 7) {
        std::cerr << "Warning: Meta ID " << current_meta_id << " for tile (" << x << "," << y << ") exceeds 3-bit capacity (0-7). Truncating." << std::endl;
        current_meta_id = 7;
      }
    } else {
      std::cerr << "Warning: Meta tile at (" << x << "," << y << ") has GID but no associated tileset for meta ID calculation." << std::endl;
    }
  }
  int palette = 0; // Assuming default palette 0 for now.

  // --- 4. Assemble the Attribute Byte (8-bit) ---
  // This will be the lower 8 bits of the combined word
  uint8_t attribute_byte = 0;
  attribute_byte |= (vFlip << 0);    // Bit 0: Vertical Flip
  attribute_byte |= (hFlip << 1);    // Bit 1: Horizontal Flip
  attribute_byte |= (palette << 2);  // Bits 2-3: Palette (assuming 2 bits, 0-3)
  attribute_byte |= (priority_flag << 4); // Bit 4: Priority (0 or 1)
  attribute_byte |= (current_meta_id << 5);     // Bits 5-7: Meta ID (assuming 3 bits, 0-7)

  // --- 5. Combine Tile ID (8-bit) and Attribute Byte (8-bit) into a single 16-bit word ---
  // From the expected output: (Tile_ID_Byte << 8) | Attribute_Byte
  uint16_t combined_word = (attribute_byte << 8) | base_tile_id_0based;

  // --- Debug Prints (Crucial for verifying this new logic) ---
  std::cout << "C++ DEBUG getTileData(" << x << "," << y << ") -> Raw GID 32bit: 0x" << std::hex << raw_gid_32bit << std::dec << std::endl;
  std::cout << "C++ DEBUG getTileData(" << x << "," << y << ") -> Base Tile ID (0-based): 0x" << std::hex << base_tile_id_0based << std::dec << std::endl;
  std::cout << "C++ DEBUG getTileData(" << x << "," << y << ") -> Meta ID: " << current_meta_id << std::endl;
  std::cout << "C++ DEBUG getTileData(" << x << "," << y << ") -> Priority: " << priority_flag << std::endl;
  std::cout << "C++ DEBUG getTileData(" << x << "," << y << ") -> HFlip: " << hFlip << ", VFlip: " << vFlip << std::endl;
  std::cout << "C++ DEBUG getTileData(" << x << "," << y << ") -> Assembled Attr Byte: 0x" << std::hex << static_cast<int>(attribute_byte) << std::dec << std::endl;
  std::cout << "C++ DEBUG getTileData(" << x << "," << y << ") -> FINAL COMBINED Word: 0x" << std::hex << combined_word << std::dec << std::endl;

  return combined_word;
}

// --- extractMetaTiles Function ---
// Extracts 2x2 metatiles from the map.
// This function remains generic and processes all metatiles in the map.
std::vector<std::array<uint16_t, 4>> extractMetaTiles(Options *opts, std::unique_ptr<tson::Map> *map) {
  tson::Map* m = map->get();
  tson::Layer* tileLayer = m->getLayer(opts->tile_layer);
  tson::Layer* priorityLayer = m->getLayer(opts->priority_layer);
  tson::Layer* metaLayer = m->getLayer(opts->meta_layer);
  tson::Vector2i size = m->getSize(); // Map size in tiles (e.g., 4x4)
  std::vector<std::array<uint16_t, 4>> all_metatiles; // Change to 4 words per metatile

  std::cout << "size: " << size.x << " x " << size.y << std::endl;

  // Iterate through the map in 2x2 blocks to form metatiles (row-major order)
  for (int x = 0; x < size.x; x += 2) {
    for (int y = 0; y < size.y; y += 2) {
      // Skip incomplete metatiles at the edges of the map
      if (x + 1 >= size.x || y + 1 >= size.y) {
        std::cerr << "Warning: Skipping incomplete metatile at (" << x << "," << y << ") due to map edge." << std::endl;
        continue;
      }

      // Get data for the four 8x8 tiles forming the 2x2 metatile
      uint16_t tl_word = getTileData(x, y, tileLayer, priorityLayer, metaLayer);     // Top-Left
      uint16_t tr_word = getTileData(x+1, y, tileLayer, priorityLayer, metaLayer);   // Top-Right
      uint16_t bl_word = getTileData(x, y+1, tileLayer, priorityLayer, metaLayer);   // Bottom-Left
      uint16_t br_word = getTileData(x+1, y+1, tileLayer, priorityLayer, metaLayer); // Bottom-Right

      // Assemble the 8-word metatile array: [TL_ID, TL_ATTRS, TR_ID, TR_ATTRS, BL_ID, BL_ATTRS, BR_ID, BR_ATTRS]
      std::array<uint16_t, 4> metatile{}; // Array of 4 words
      metatile[0] = tl_word;
      metatile[1] = tr_word;
      metatile[2] = bl_word;
      metatile[3] = br_word;

      std::cout << "C++ DEBUG Assembled Metatile for (" << x << "," << y << "): {";
      for (size_t i = 0; i < metatile.size(); ++i) {
        std::cout << "0x" << std::hex << metatile[i];
        if (i < metatile.size() - 1) std::cout << ", ";
      }
      std::cout << std::dec << "}" << std::endl;

      all_metatiles.push_back(metatile);
    }
  }

  // Deduplication (commented out as per previous debugging steps to isolate issues)
  // std::sort(all_metatiles.begin(), all_metatiles.end());
  // auto last = std::unique(all_metatiles.begin(), all_metatiles.end());
  // all_metatiles.erase(last, all_metatiles.end());

  return all_metatiles;
}

void saveMetatileFile(std::vector<std::array<uint16_t, 4>> metatiles, std::string filename) {
  // Calculate total file length (8 bytes header + metatiles.size() * 4 words/metatile * 2 bytes/word).
  // If your map is 4x4, extractMetaTiles will produce 4 metatiles.
  // So, metatiles.size() will be 4.
  // Total data bytes = 4 * 4 * 2 = 32 bytes.
  // Total file length = 32 bytes (data) + 8 bytes (header) = 40 bytes.
  uint16_t total_file_length = static_cast<uint16_t>(8 + metatiles.size() * 4 * 2);

  std::ofstream ofs(filename, std::ios::binary);
  if (!ofs) {
    std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
    return;
  }

  // --- Write the 8-byte header ---
  ofs.put(static_cast<char>(total_file_length & 0xFF));        // LSB (Lower byte of length)
  ofs.put(static_cast<char>((total_file_length >> 8) & 0xFF)); // MSB (Upper byte of length)
  for (int i = 0; i < 6; ++i) {
    ofs.put(0x00); // Write a null byte
  }

  // --- Write the metatile data ---
  // Each uint16_t word needs to be written as 2 bytes in BIG-ENDIAN order.
  // We write all metatiles generated from the map.
  for (const auto& metatile : metatiles) { // <<< THIS IS THE CORRECT LOOP
    for (uint16_t val : metatile) { // Iterate over its 4 uint16_t words
      ofs.put(static_cast<char>(val & 0xFF));        // Lower byte (LSB)
      ofs.put(static_cast<char>((val >> 8) & 0xFF)); // Upper byte (MSB)
    }
  }
  ofs.close();
}

// --- processTiledDoc Function ---
// Main function to process the Tiled map and extract/save metatiles.
int processTiledDoc(Options *opts) {
  std::cout << "[processTiledDoc] Processing... " << opts->input_file << std::endl;

  // Parse the Tiled file using Tileson
  tson::Tileson t;
  std::unique_ptr<tson::Map> map = t.parse(opts->input_file);
  if (map->getStatus() != tson::ParseStatus::OK) {
    std::cerr << "Failed to parse Tiled map: " << opts->input_file << std::endl;
    return 1;
  }

  // Extract the metatiles from the parsed map
  std::vector<std::array<uint16_t, 4>> metatiles = extractMetaTiles(opts, &map);
  std::cout << "metatile count: " << metatiles.size() << std::endl;

  // Save the metatiles to a file if requested
  if (opts->save_metatiles_file != "") {
    saveMetatileFile(metatiles, opts->save_metatiles_file);
  }

  return 0;
}
