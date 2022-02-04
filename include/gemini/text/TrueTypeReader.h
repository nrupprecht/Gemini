//
// Created by Nathaniel Rupprecht on 12/7/21.
//

// The OpenType documentation
//    https://docs.microsoft.com/en-us/typography/opentype/spec/
// The TrueType documentation
//    https://developer.apple.com/fonts/TrueType-Reference-Manual/
// Thanks to the helpful articles:
//    http://stevehanov.ca/blog/?id=143
//    https://tchayen.github.io/posts/ttf-file-parsing

#ifndef GEMINI_INCLUDE_GEMINI_TRUETYPEREADER_H_
#define GEMINI_INCLUDE_GEMINI_TRUETYPEREADER_H_

#include "gemini/core/shapes/BezierCuve.h"
#include <map>
#include <set>
#include <cmath>
#include <fstream>
#include <ostream>

namespace gemini::text {

//! \brief Render the index of all the tables in the TrueType font as a table.
void GEMINI_EXPORT PrintTableIndex(std::ostream& out, const class TrueType& font);


using uint8 = uint8_t;
using int8 = int8_t;
using uint16 = uint16_t;
using int16 = int16_t;
using uint32 = uint32_t;
using int32 = int32_t;
using Fixed = int32_t;
using FWORD = int16;
using UFWORD = uint16;
using F2DOT14 = uint16;
using LONGDATETIME = int64_t;
using Tag = uint8[4];
using Offset16 = uint16;
using Offset32 = uint32;
using Version16Dot16 = uint32;

enum GlyfFlags : uint8 {
  ON_CURVE  = 0b00000001,
  X_IS_BYTE = 0b00000010,
  Y_IS_BYTE = 0b00000100,
  REPEAT    = 0b00001000,
  X_DELTA   = 0b00010000,
  Y_DELTA   = 0b00100000,
};


//! \brief Class that encodes a TrueType font.
class GEMINI_EXPORT TrueType {
  friend class TrueTypeFontEngine;
 public:
  //! \brief Data encoding information about a TrueType table.
  struct Table {
    uint32 checksum;
    uint32 offset;
    uint32 length;
  };

  using Tables = std::map<std::string, Table>;

  //! \brief Read a TrueType font from a .ttf file (or any file containing true type).
  void ReadTTF(const std::string &filename);

  NO_DISCARD std::vector<uint16> GetAllDefinedCharacters() const;

  NO_DISCARD unsigned int GetNumGlyphs() const;

  NO_DISCARD const Tables& GetTables() const;

 private:

  //! \brief A helper function for reading data from the file.
  template<std::size_t counter, typename Integer_t>
  void read(Integer_t &data) const {
    auto ptr = &reinterpret_cast<char *>(&data)[sizeof(Integer_t) - counter - 1];
    *ptr = file_[file_ptr_++];
    if constexpr (counter + 1 < sizeof(Integer_t)) {
      read<counter + 1>(data);
    }
  }

  //! \brief Read data of a specified type from the file.
  template<typename Integer_t>
  Integer_t read() const {
    Integer_t data;
    read<0>(data);
    return data;
  }

  template<typename Integer_t>
  Integer_t read_at(uint32 location) const {
    auto save_ptr = file_ptr_;
    seek(location);
    auto data = read<Integer_t>();
    seek(save_ptr);
    return data;
  }

  //! \brief Extract a string from the file.
  NO_DISCARD std::string getString(int num_chars) const;

  //! \brief Move the filepointer. Returns the location before the function was called.
  uint32 seek(uint32 new_position) const;

  //! \brief Move the filepointer, relative to the current position.
  //! Returns the location before the function was called.
  uint32 advance(int16 d_position);

  //! \brief Calculate a checksum.
  uint32 calcCheckSum(uint32 offset, uint32 length);

  //! \brief Read the HEAD table.
  void readHeadTable();

  //! \brief Read the MAXP table.
  void readMAXPTable();

  //! \brief Read the HHEA table.
  void readHHEATable();

  //! \brief Read the HMTX table. Must be done after reading the HHEA table and MAXP table.
  void readHMTXTable();

  //! \brief Read the LOCA table.
  void readLOCATable();

  //! \brief Read the GLYF table.
  void readGLYFTable();

  //! \brief Read the CMAP table.
  //!
  //! See documentation: https://docs.microsoft.com/en-us/typography/opentype/spec/cmap.
  void readCMAPTable();

  //! \brief Get all the glyph spacing information.
  void getSpacingInformation();

  // =========================================================================
  //  Private variables.
  // =========================================================================



  // From head table
  struct HeadTable {
    Fixed version;
    Fixed font_revision;
    uint32 checksum_adjustment;
    uint32 magic_number;
    uint16 flags;
    uint16 units_per_em;
    LONGDATETIME created;
    LONGDATETIME modified;
    FWORD xmin, ymin, xmax, ymax;
    uint16 mac_style;
    uint16 lowest_rec_PPEM;
    int16 font_direction_hint;
    int16 index_to_loc_format;
    int16 glyph_data_format;
  } head_table_;

  // From maxp table
  struct MAXPTable {
    Fixed version;
    uint16 num_glyphs;
    uint16 max_points;
    uint16 max_contours;
    uint16 max_composite_points;
    uint16 max_composite_contours;
    uint16 max_zones;
    uint16 max_twilight_points;
    uint16 max_storage;
    uint16 max_function_defs;
    uint16 max_instruction_defs;
    uint16 max_stack_elements;
    uint16 max_size_of_instructions;
    uint16 max_component_elements;
    uint16 max_component_depth;
  } maxp_table_;

  // From hhea table.
  struct HHEATable {
    Fixed version;
    FWORD ascent;
    FWORD descent;
    FWORD line_gap;
    UFWORD advance_width_max;
    FWORD min_left_side_bearing;
    FWORD min_right_side_bearings;
    FWORD x_max_extent;
    int16 caret_slope_rise;
    int16 care_slope_run;
    FWORD caret_offset;
    int16 metric_data_format;
    uint16 num_of_long_hor_metrics;
  } hhea_table_;

  struct LongHorMetric {
    uint16 advance_width;
    int16 left_side_bearing;
  };

  // From hmtx table.
  struct HMTXTable {
    std::vector<LongHorMetric> hmetrics;
    std::vector<int16> left_side_bearings;
  } hmtx_table_;

  // HDMX table (horizontal device metrics). Only for MAC fonts.
  struct HDMXTable {} hdmx_table_;

  struct LOCATable {
    std::vector<uint32> entries;
  } loca_table_;

  //! \brief Whether a glyph is simple or complex.
  enum class GlyphType : uint8 { Simple, Compound };


  //! \brief Helper struct for GLYF table.
  struct GlyphData {
    int16 number_of_contours{}, xmin{}, ymin{}, xmax{}, ymax{};

    GlyphType type = GlyphType::Simple;

    shapes::BezierCurve spline;

    std::vector<uint8> instructions_{};

    bool simple_overlap = false;
  };

  struct GLYFTable {
    std::vector<GlyphData> entries;
  } glyf_table_;

  //! \brief Helper struct for CMAP table.
  struct CMAPEncoding {
    uint16 platform_id;
    uint16 encoding_id;
    uint32 offset;
  };

  struct CMAPTable {
    uint16 version;
    uint16 format;
    uint16 num_tables;
    std::vector<CMAPEncoding> encoding_records;

    //! \brief Map from unicode -> glyph index.
    std::map<uint16, uint16> glyph_index_map;
  } cmap_table_;

  //! \brief Encodes "format 4," the segment mapping to delta values format.
  struct Format4Data {
    uint16 length{};
    uint16 language{};
    uint16 seg_count{};
    uint16 search_range{};
    uint16 entry_selector{};
    uint16 range_shift{};
    std::vector<uint16> end_code;
    std::vector<uint16> start_code;
    std::vector<int16> id_delta;
    std::vector<uint16> id_range_offset;

    // Mapping from character to glyph index.
    std::map<uint16, uint16> glyph_index_map;
  };

  struct SpacingInfo {
    int16 xmin, ymin;
    uint16 width, height;
    int16 lsb, rsb;
    uint16 advance;
  };

  //! \brief Parse format data in format 4.
  //! Reference: https://docs.microsoft.com/en-us/typography/opentype/spec/cmap#format-4-segment-mapping-to-delta-values.
  Format4Data parseFormat4();

 private:
  //! \brief String (array of chars) representing the entire file.
  std::string file_;

  //! \brief Pointer to the current location in the file.
  mutable uint32 file_ptr_ = 0;

  //! \brief Record of tables
  Tables tables_;

  //! \brief Glyph index to spacing information.
  std::map<uint16, SpacingInfo> spacing_map_;

 public:

  const SpacingInfo& GetSpacing(uint16 glyph_index) const {
    auto it = spacing_map_.find(glyph_index);
    if (it != spacing_map_.end()) {
      return it->second;
    }

    // Otherwise, return the missing character glyph.
    return spacing_map_.find(0)->second; // Something random.
  }
};


//inline void PrintTableIndex(std::ostream& out, const TrueType& font) {
//  // Print the set of tables.
//  int count = 0;
//  out << "┌─────┬────────┬──────────┬──────────┐" << std::endl;
//  out << std::left << std::setw(5) << "│  #  │"
//      << std::left << std::setw(8) << " Tag"
//      << std::setw(13) << "│ Length" << std::setw(10) << "│ Offset   │" << std::endl;
//  out << "├─────┼────────┼──────────┼──────────┤" << std::endl;
//  for (const auto& [name, table] : font.GetTables()) {
//    out << "│ " << std::left << std::setw(4) << std::to_string(count)
//        << "│ " << std::setw(7) << name
//        << "│ " << std::setw(9) << table.length
//        << "│ " << std::setw(9) << table.offset << "│" << std::endl;
//    ++count;
//  }
//  out << "└─────┴────────┴──────────┴──────────┘" << std::endl;
//}

} // namespace gemini::text

#endif //GEMINI_INCLUDE_GEMINI_TRUETYPEREADER_H_
