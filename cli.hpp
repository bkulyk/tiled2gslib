#ifndef T2G_CLI_HPP
#define T2G_CLI_HPP

#include <filesystem>
#include <iostream>
#include <string>

#include "./lib/CLI11.hpp"

struct Options {
  std::string input_file;
  std::string input_type;
  std::string save_tiles_file;
  std::string save_metatiles_file;
  std::string tilesize = "8x8";
  std::string palette = "sms";
  std::string priority_layer = "GSLPriorityLayer";
  std::string tile_layer = "GSLTileLayer";
  std::string meta_layer = "GSLMetaLayer";

  int tileoffset = 0;
  int metaoffset = 96;
  bool remove_dupes = false;
};

// ---
// Important: Overload operator<< for your Options struct
// This tells pprint (and std::cout) how to print an Options object
std::ostream& operator<<(std::ostream& os, const Options& opts) {
  // We can use pprint's own formatting for strings, bools, ints, etc.
  // by building a string or by printing directly.
  // For a structured output, list each member explicitly.
  os << "{\n"
      << "  input_file: \"" << opts.input_file << "\",\n"
      << "  input_type: \"" << opts.input_type << "\",\n"
      << "  save_tiles_file: \"" << opts.save_tiles_file << "\",\n"
      << "  save_metatiles_file: \"" << opts.save_metatiles_file << "\",\n"
      << "  tilesize: \"" << opts.tilesize << "\",\n"
      << "  tileoffset: " << opts.tileoffset << ",\n"
      << "  palette: \"" << opts.palette << "\",\n"
      << "  remove_dupes: " << (opts.remove_dupes ? "true" : "false") << ",\n"
      << "  priority_layer: \"" << opts.priority_layer << "\",\n"
      << "  tile_layer: \"" << opts.tile_layer << "\",\n"
      << "  meta_layer: \"" << opts.meta_layer << "\"\n"
      << "}";
  return os;
}
// ---

Options parse_options(int argc, char** argv) {
  CLI::App app{"tiled2gslib - Convert a .tmx or .png file for GSLib"};
  Options opts;

  app.add_option("input", opts.input_file, "Input file (.tmj or .png)")->required()->check(CLI::ExistingFile);

  app.add_option("--save-tiles", opts.save_tiles_file, "Optional output file path for tiles");
  app.add_option("--save-metatiles", opts.save_metatiles_file, "Optional output file path for metatiles");
  app.add_option("--tilesize", opts.tilesize, "Tile size: 8x8 (default) or 8x16")->check(CLI::IsMember({"8x8", "8x16"}));
  app.add_option("--tileoffset", opts.tileoffset, "Tile offset (default: 0)");
  app.add_option("--palette", opts.palette, "Palette: sms (default) or gg")->check(CLI::IsMember({"sms", "gg"}));
  app.add_option("--tile-layer", opts.tile_layer, "Tile layer name (default: GSLTileLayer)");
  app.add_option("--priority-layer", opts.priority_layer, "Priority layer name (default: GSLPriorityLayer)");
  app.add_option("--meta-layer", opts.meta_layer, "Meta layer name (default: GSLMetaLayer)");

  app.add_flag("--remove-dupes", opts.remove_dupes, "Remove duplicate tiles (default: false)");

  try {
    app.parse(argc, argv);
    std::filesystem::path p(opts.input_file);
    opts.input_type = p.extension().string();
  } catch (const CLI::ParseError &e) {
    std::exit(app.exit(e));
  }

  return opts;
}

#endif
