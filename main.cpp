#include <string>
#include <stdio.h>

#include "./cli.hpp"
#include "./tiled.hpp"

int main(int argc, char** argv) {
  Options opts = parse_options(argc, argv);

  if (opts.input_type == ".tmx") {
    std::cout << ".tmx not supported file, open in Tiled, save as .tmj";
    return 1;
  } else if (opts.input_type == ".tmj") {
    processTiledDoc(&opts);
  } else if (opts.input_type == ".png" ) {
    std::cout << ".png not supported yet (TODO)";
    return 1;
  } else {
    std::cout << "input type not supported see --help";
    return 1;
  }

  return 0;
}
