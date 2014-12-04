/*!\defgroup OPTIONS*/
/******************************************************************************/
/*!
 \file       options.cpp
 \brief
 
 \version
 20.11.2014 - Nicola Corna - nicola@corna.info\n
 - added bridge height option

 \version
 19.12.2013 - Erik Schuster - erik@muenchen-ist-toll.de\n
 - added options "bridges" and "optimise".

 \version
 2013 - Erik Schuster - erik@muenchen-ist-toll.de\n
 - Added a check and warning if a drill file is given, but no board file or the
 absolute-mirror option.
 - Added cut-front option.
 - Added metricoutput option.
 - Added g64 option.
 - Added onedrill option.
 - Prepared documenting the code with doxygen.
 - Formatted the code.

 \version
 1.1.4 - 2009, 2010 Patrick Birnzain <pbirnzain@users.sourceforge.net> and others

 \copyright  pcb2gcode is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 pcb2gcode is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with pcb2gcode.  If not, see <http://www.gnu.org/licenses/>.

 \ingroup    OPTIONS
 */
/******************************************************************************/

#include "options.hpp"
#include "config.h"

#include <fstream>
#include <list>
#include <boost/foreach.hpp>
#include <boost/exception/all.hpp>

#include <iostream>
using std::cerr;
using std::endl;

/******************************************************************************/
/*
 */
/******************************************************************************/
options&
options::instance() {
   static options singleton;
   return singleton;
}

/******************************************************************************/
/*
 */
/******************************************************************************/
void options::parse(int argc, char** argv) {
   // guessing causes problems when one option is the start of another
   // (--drill, --drill-diameter); see bug 3089930
   int style = po::command_line_style::default_style
               & ~po::command_line_style::allow_guessing;

   po::options_description generic;
   generic.add(instance().cli_options).add(instance().cfg_options);

   try {
      po::store(po::parse_command_line(argc, argv, generic, style),
                instance().vm);
   } catch (std::logic_error& e) {
      cerr << "Error: You've supplied an invalid parameter.\n"
           << "Note that spindle speeds are integers!\n" << "Details: "
           << e.what() << endl;
      exit(101);
   }

   po::notify(instance().vm);

   parse_files();

   /*
    * this needs to be an extra step, as --basename modifies the default
    * values of the --...-output parameters
    */
   string basename = "";

   if (instance().vm.count("basename")) {
      basename = instance().vm["basename"].as<string>() + "_";
   }

   string front_output = "--front-output=" + basename + "front.ngc";
   string back_output = "--back-output=" + basename + "back.ngc";
   string outline_output = "--outline-output=" + basename + "outline.ngc";
   string drill_output = "--drill-output=" + basename + "drill.ngc";

   const char *fake_basename_command_line[] = { "", front_output.c_str(),
                                                back_output.c_str(),
                                                outline_output.c_str(),
                                                drill_output.c_str() };

   po::store(
            po::parse_command_line(5, (char**) fake_basename_command_line,
                                   generic, style),
            instance().vm);
   po::notify(instance().vm);
}

/******************************************************************************/
/*
 */
/******************************************************************************/
string options::help() {
   std::stringstream msg;
   msg << PACKAGE_STRING << "\n\n";
   msg << instance().cli_options << instance().cfg_options;
   return msg.str();
}

/******************************************************************************/
/*
 */
/******************************************************************************/
void options::parse_files() {

   std::string file("millproject");

   try {
      std::ifstream stream;

      try {
         stream.open(file.c_str());
         po::store(po::parse_config_file(stream, instance().cfg_options),
                   instance().vm);
      } catch (std::exception& e) {
         cerr << "Error parsing configuration file \"" << file << "\": "
              << e.what() << endl;
      }

      stream.close();
   } catch (std::exception& e) {
      cerr << "Error reading configuration file \"" << file << "\": "
           << e.what() << endl;
   }

   po::notify(instance().vm);
}

/******************************************************************************/
/*
 */
/******************************************************************************/
options::options()
         : cli_options("command line only options"),
           cfg_options("generic options (CLI and config files)") {

   cli_options.add_options()("help,?", "produce help message")("version", "\n");

   cfg_options.add_options()("front", po::value<string>(),
                             "front side RS274-X .gbr")(
            "back", po::value<string>(), "back side RS274-X .gbr")(
            "outline", po::value<string>(), "pcb outline polygon RS274-X .gbr")(
            "drill", po::value<string>(), "Excellon drill file\n")(
            "svg", po::value<string>(), "SVG output file. EXPERIMENTAL\n")(
            "zwork", po::value<double>(),
            "milling depth in inches (Z-coordinate while engraving)")(
            "zsafe", po::value<double>(),
            "safety height (Z-coordinate during rapid moves)")(
            "offset",
            po::value<double>(),
            "distance between the PCB traces and the end mill path in inches; usually half the isolation width")(
            "mill-feed", po::value<double>(),
            "feed while isolating in [i/m] or [mm/m]")(
            "mill-speed", po::value<int>(), "spindle rpm when milling")(
            "milldrill", "drill using the mill head")(
            "extra-passes",
            po::value<int>(),
            "specify the the number of extra isolation passes, increasing the isolation width half the tool diameter with each pass\n")(
            "fill-outline", po::value<bool>()->zero_tokens(),
            "accept a contour instead of a polygon as outline")(
            "outline-width", po::value<double>(), "width of the outline")(
            "cutter-diameter", po::value<double>(),
            "diameter of the end mill used for cutting out the PCB")(
            "zcut", po::value<double>(), "PCB cutting depth in inches")(
            "cut-feed", po::value<double>(),
            "PCB cutting feed in [i/m] or [mm/m]")("cut-speed",
                                                   po::value<int>(),
                                                   "spindle rpm when cutting")(
            "cut-infeed", po::value<double>(),
            "maximum cutting depth; PCB may be cut in multiple passes")(
            "cut-front", po::value<bool>()->zero_tokens(),
            "Cut from front side. Default is back side.\n")("zdrill",
                                                            po::value<double>(),
                                                            "drill depth")(
            "zchange", po::value<double>(), "tool changing height")(
            "drill-feed", po::value<double>(), "drill feed in [i/m] or [mm/m]")(
            "drill-speed", po::value<int>(), "spindle rpm when drilling")(
            "drill-front", po::value<bool>()->zero_tokens(),
            "drill through the front side of board")(
            "onedrill",
            po::value<bool>()->default_value(false)->zero_tokens()
                     ->implicit_value(true),
            "use only one drill bit size\n")(
            "metric",
            po::value<bool>()->default_value(false)->zero_tokens()
                     ->implicit_value(true),
            "use metric units for parameters. does not affect gcode output")(
            "metricoutput",
            po::value<bool>()->default_value(false)->zero_tokens()
                     ->implicit_value(true),
            "use metric units for output")(
            "optimise",
            po::value<bool>()->default_value(false)->zero_tokens()
                     ->implicit_value(true),
            "Reduce output file size by up to 40% while accepting a little loss of precision.")(
            "bridges", po::value<double>()->default_value(0),
            "add four bridges with the given width to the outline cut")(
            "zbridges", po::value<double>(),
            "bridges heigth (Z-coordinates while engraving bridges, default to zsafe) ")(
            "dpi", po::value<int>()->default_value(1000),
            "virtual photoplot resolution")(
            "g64", po::value<double>(),
            "maximum deviation from toolpath, overrides internal calculation")(
            "mirror-absolute", po::value<bool>()->zero_tokens(),
            "mirror back side along absolute zero instead of board center\n")(
            "basename", po::value<string>(),
            "prefix for default output file names")(
            "front-output", po::value<string>()->default_value("front.ngc"),
            "output file for front layer")(
            "back-output", po::value<string>()->default_value("back.ngc"),
            "output file for back layer")(
            "outline-output", po::value<string>()->default_value("outline.ngc"),
            "output file for outline")(
            "drill-output", po::value<string>()->default_value("drill.ngc"),
            "output file for drilling\n")(
            "preamble-text", po::value<string>(),
            "preamble text file, inserted at the very beginning as a comment.")(
            "preamble", po::value<string>(),
            "gcode preamble file, inserted at the very beginning.")(
            "postamble", po::value<string>(),
            "gcode postamble file, inserted before M9 and M2.");
}

/******************************************************************************/
/*
 */
/******************************************************************************/
static void check_generic_parameters(po::variables_map const& vm) {

   //---------------------------------------------------------------------------
   //Check dpi parameter:

   if (vm["dpi"].as<int>() < 100) {
      cerr << "Warning: very low DPI value." << endl;
   } else
      if (vm["dpi"].as<int>() > 10000) {
         cerr << "Warning: very high DPI value, processing may take extremely long"
              << endl;
      }

   //---------------------------------------------------------------------------
   //Check g64 parameter:

   double g64th;      //!< Upper threshold value of g64 parameter for warning

   if (vm.count("g64")) {                  //g64 parameter is given
      if (vm["metric"].as<bool>()) {       //metric input
         g64th = 0.1;                      //set threshold value
      } else {                             //imperial input
         g64th = 0.003937008;              //set threshold value
      }

      if (vm["g64"].as<double>() > g64th)
         cerr << "Warning: high G64 value (allowed deviation from toolpath) given.\n"
              << endl;

      if (vm["g64"].as<double>() == 0)
         cerr << "Warning: Deviation from commanded toolpath set to 0 (g64=0). No smooth milling is most likely!\n"
              << endl;
   }

   //---------------------------------------------------------------------------
   //Check for available board dimensions:

   if (vm.count("drill")
       && !(vm.count("front") || vm.count("back") || vm.count("outline"))
       && !vm.count("mirror-absolute")) {
      cerr << "Warning: Board dimensions unknown. Gcode for drilling probably misaligned on Y-axis.\n";
   }

   //---------------------------------------------------------------------------
   //Check for safety height parameter:

   if (!vm.count("zsafe")) {
      cerr << "Error: Safety height not specified.\n";
      exit(5);
   }

   //---------------------------------------------------------------------------
   //Check for zchange parameter parameter:

   if (!vm.count("zchange")) {
      cerr << "Error: Tool changing height not specified.\n";
      exit(15);
   }
}

/******************************************************************************/
/*
 */
/******************************************************************************/
static void check_milling_parameters(po::variables_map const& vm) {

   if (vm.count("front") || vm.count("back")) {

      if (!vm.count("zwork")) {
         cerr << "Error: --zwork not specified.\n";
         exit(1);
      } else
         if (vm["zwork"].as<double>() > 0) {
            cerr << "Warning: Engraving depth (--zwork) is greater than zero!\n";
         }

      if (!vm.count("offset")) {
         cerr << "Error: Engraving --offset not specified.\n";
         exit(4);
      }

      if (!vm.count("mill-feed")) {
         cerr << "Error: Milling feed [ipm] not specified.\n";
         exit(13);
      }

      if (!vm.count("mill-speed")) {
         cerr << "Error: Milling speed [rpm] not specified.\n";
         exit(14);
      }

      // required parameters present. check for validity.
      if (vm["zsafe"].as<double>() <= vm["zwork"].as<double>()) {
         cerr << "Error: The safety height --zsafe is lower than the milling "
              << "height --zwork. Are you sure this is correct?\n";
         exit(15);
      }

      if (vm["mill-feed"].as<double>() < 0) {
         cerr << "Error: Negative milling feed (--mill-feed).\n";
         exit(17);
      }

      if (vm["mill-speed"].as<int>() < 0) {
         cerr << "Error: --mill-speed < 0.\n";
         exit(16);
      }
   }
}

/******************************************************************************/
/*
 */
/******************************************************************************/
static void check_drilling_parameters(po::variables_map const& vm) {

   //only check the parameters if a drill file is given
   if (vm.count("drill")) {

      if (!vm.count("zdrill")) {
         cerr << "Error: Drilling depth (--zdrill) not specified.\n";
         exit(9);
      }

      if (vm["zsafe"].as<double>() <= vm["zdrill"].as<double>()) {
         cerr << "Error: The safety height --zsafe is lower than the drilling "
              << "height --zdrill!\n";
         exit(18);
      }

      if (!vm.count("zchange")) {
         cerr << "Error: Drill bit changing height (--zchange) not specified.\n";
         exit(10);
      } else
         if (vm["zchange"].as<double>() <= vm["zdrill"].as<double>()) {
            cerr << "Error: The safety height --zsafe is lower than the tool "
                 << "change height --zchange!\n";
            exit(19);
         }

      if (!vm.count("drill-feed")) {
         cerr << "Error:: Drilling feed (--drill-feed) not specified.\n";
         exit(11);
      } else
         if (vm["drill-feed"].as<double>() <= 0) {
            cerr << "Error: The drilling feed --drill-feed is <= 0.\n";
            exit(20);
         }

      if (!vm.count("drill-speed")) {
         cerr << "Error: Drilling spindle RPM (--drill-speed) not specified.\n";
         exit(12);
      } else
         if (vm["drill-speed"].as<int>() < 0) {      //no need to support both directions?
            cerr << "Error: --drill-speed < 0.\n";
            exit(17);
         }

   }

}

/******************************************************************************/
/*
 */
/******************************************************************************/
static void check_cutting_parameters(po::variables_map const& vm) {

   //only check the parameters if an outline file is given
   if (vm.count("outline")) {

      if ((vm.count("drill") && vm.count("milldrill"))) {
         if (vm.count("fill-outline")) {
            if (!vm.count("outline-width")) {
               cerr << "Error: For outline filling, a width (--outline-width) has to be specified.\n";
               exit(25);
            } else {
               double outline_width = vm["outline-width"].as<double>();
               if (outline_width < 0) {
                  cerr << "Error: Specified outline width is less than zero!\n";
                  exit(26);
               } else
                  if (outline_width == 0) {
                     cerr << "Error. Specified outline width is zero!\n";
                     exit(27);
                  } else {
                     std::stringstream width_sb;
                     if ((vm.count("metric") && outline_width >= 10)
                         || (!vm.count("metric") && outline_width >= 0.4)) {
                        width_sb << outline_width
                                 << (vm.count("metric") ? " mm" : " inch");
                        cerr << "Warning: You specified an outline-width of "
                             << width_sb.str() << "!\n";
                     }
                  }
            }
         }
      }

      if (!vm.count("zcut")) {
         cerr << "Error: Board cutting depth (--zcut) not specified.\n";
         exit(5);
      }

      if (!vm.count("cutter-diameter")) {
         cerr << "Error: Cutter diameter not specified.\n";
         exit(15);
      }

      if (!vm.count("cut-feed")) {
         cerr << "Error: Board cutting feed (--cut-feed) not specified.\n";
         exit(6);
      }

      if (!vm.count("cut-speed")) {
         cerr << "Error: Board cutting spindle RPM (--cut-speed) not specified.\n";
         exit(7);
      }

      if (!vm.count("cut-infeed")) {
         cerr << "Error: Board cutting infeed (--cut-infeed) not specified.\n";
         exit(8);
      }

      if (vm["zsafe"].as<double>() <= vm["zcut"].as<double>()) {
         cerr << "Error: The safety height --zsafe is lower than the cutting "
              << "height --zcut!\n";
         exit(21);
      }

      if (vm["cut-feed"].as<double>() <= 0) {
         cerr << "Error: The cutting feed --cut-feed is <= 0.\n";
         exit(22);
      }

      if (vm["cut-speed"].as<int>() < 0) {      //no need to support both directions?
         cerr << "Error: The cutting spindle speed --cut-speed is lower than 0.\n";
         exit(23);
      }

      if (vm["cut-infeed"].as<double>() < 0.001) {
         cerr << "Error: The cutting infeed --cut-infeed. seems too low.\n";
         exit(24);
      }
   }
}

/******************************************************************************/
/*
 */
/******************************************************************************/
void options::check_parameters() {

   po::variables_map const& vm = instance().vm;

   try {
      check_generic_parameters(vm);
      check_milling_parameters(vm);
      check_cutting_parameters(vm);
      check_drilling_parameters(vm);
   } catch (std::runtime_error& re) {
      cerr << "Error: Invalid parameter. :-(\n";
      exit(100);
   }
}
