//
// Created by Nathaniel Rupprecht on 12/7/21.
//

#include "gemini/text/TrueTypeReader.h"
// Other files.
#include <fstream>

#include <iostream>

using namespace gemini::text;

namespace gemini::text {

void PrintTableIndex(std::ostream& out, const TrueType& font) {

  auto& table_map = font.GetTables();
  std::vector<std::pair<uint32_t, std::string>> sortable;
  sortable.reserve(table_map.size());
  for (auto [name, table] : table_map) {
    sortable.emplace_back(table.offset, name);
  }
  std::sort(sortable.begin(), sortable.end());

  // Print the set of tables.
  int count = 0;
  out << "┌────┬────────┬──────────┬──────────┐" << std::endl;
  out << std::left << std::setw(4) << "│ #  │"
      << std::left << std::setw(8) << " Tag"
      << std::setw(13) << "│ Length" << std::setw(10) << "│ Offset   │" << std::endl;
  out << "├────┼────────┼──────────┼──────────┤" << std::endl;
  for (const auto& [_, name] : sortable) {
    auto& table = table_map.at(name);
    out << "│" << std::left << std::setw(4) << (" " + std::to_string(count))
        << "│" << std::setw(8) << name
        << "│" << std::setw(10) << table.length
        << "│" << std::setw(10) << table.offset << "│" << std::endl;
    ++count;
  }
  out << "└────┴────────┴──────────┴──────────┘" << std::endl;
}

} // namespace gemini::text


void TrueType::ReadTTF(const std::string &filename) {
  std::ifstream fin(filename);

  if (fin.fail()) {
    GEMINI_FAIL("could not open the file \"" << filename << "\"");
  }

  // Read in the entire file.
  std::copy(std::istreambuf_iterator<char>(fin),std::istreambuf_iterator<char>(), std::back_inserter(file_));
  fin.close();

  // Read initial data.
  auto sfnt_version = read<uint32>();
  auto num_tables = read<uint16>();
  auto search_range = read<uint16>();
  auto entry_selector = read<uint16>();
  auto range_shift = read<uint16>();

  bool is_sfnt = sfnt_version == 0x00010000;
  bool is_otto = sfnt_version == 0x4F54544F;
  GEMINI_ASSERT(is_sfnt || is_otto, "sfnt version unknown");

  // Check search range.
  auto sr = std::pow(2., floor(std::log(num_tables) / std::log(2.))) * 16;
  GEMINI_ASSERT(sr == search_range, "range shift incorrect");

  // Check entry select
  auto es = std::floor(std::log(num_tables) / std::log(2.));
  GEMINI_ASSERT(es == entry_selector, "entry selector incorrect");

  // Check range shift.
  auto rs = (16 * num_tables) - search_range;
  GEMINI_ASSERT(rs == range_shift, "range shift incorrect");

  // Read tables index.
  for (int i = 0; i < num_tables; ++i) {
    std::string tag = getString(4);
    auto checksum = read<uint32>();
    auto offset = read<uint32>();
    auto length = read<uint32>();
    tables_[tag] = Table{checksum, offset, length};

    // Make sure check sum is correct.
    if (tag != "head") {
      auto check = calcCheckSum(offset, length);
      GEMINI_ASSERT(check == checksum, "checksum for table Tag = " << tag << " is incorrect");
    }
  }

  for (auto [tag, table] : tables_) {
    std::cout << tag << std::endl;
  }

  // ============================================================================================================
  // The required tables (for OpenType or TrueType) are cmap, head, hhea, hmtx, maxp, name, OS/2, post.
  // ============================================================================================================

  GEMINI_ASSERT(tables_.find("head") != tables_.end(), "missing required table \"head\"");
  readHeadTable();

  // Name table.
  // *  https://docs.microsoft.com/en-us/typography/opentype/spec/name
  GEMINI_ASSERT(tables_.find("name") != tables_.end(), "missing required table \"name\"");
  readNAMETable();

  // Maximum profile table. Establishes the memory requirements for this font.
  // *  https://docs.microsoft.com/en-us/typography/opentype/spec/maxp
  GEMINI_ASSERT(tables_.find("maxp") != tables_.end(), "missing required table \"maxp\"");
  readMAXPTable();

  // Horizontal header table
  // *  https://docs.microsoft.com/en-us/typography/opentype/spec/hhea
  GEMINI_ASSERT(tables_.find("hhea") != tables_.end(), "missing required table \"hhea\"");
  readHHEATable();

  // Horizontal metrics table. Depends on the maxp table.
  // *  https://docs.microsoft.com/en-us/typography/opentype/spec/hmtx
  GEMINI_ASSERT(tables_.find("hmtx") != tables_.end(), "missing required table \"hmtx\"");
  readHMTXTable();

  // Character to Glyph Index Mapping table.
  // *  https://docs.microsoft.com/en-us/typography/opentype/spec/cmap#format-4-segment-mapping-to-delta-values
  // *  https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6cmap.html
  GEMINI_ASSERT(tables_.find("cmap") != tables_.end(), "missing required table \"cmap\"");
  readCMAPTable();

  // OS/2 and Windows metrics table.
  // *  https://docs.microsoft.com/en-us/typography/opentype/spec/os2
  GEMINI_ASSERT(tables_.find("OS/2") != tables_.end(), "missing required table \"OS/2\"");

  // Postscript information table.
  // *  https://docs.microsoft.com/en-us/typography/opentype/spec/post
  GEMINI_ASSERT(tables_.find("post") != tables_.end(), "missing required table \"post\"");
  readPOSTTable();

  // ============================================================================================================
  // Other tables.
  // ============================================================================================================

  // Depends on the maxp table.
  // GEMINI_ASSERT(tables_.find("loca") != tables_.end(), "missing required table \"loca\"");
  if (tables_.find("loca") != tables_.end()) {
    readLOCATable();
  }

  // Glyph data.
  if (tables_.find("glyf") != tables_.end()) {
    readGLYFTable();
  }

  // Font Program
  // readFPGMTable();

  // Control value table.
  // readCVTTable();

  getSpacingInformation();
}

std::vector<uint16> TrueType::GetAllDefinedCharacters() const {
  std::vector<uint16> output;
//  output.reserve(cmap_table_.glyph_index_map.size());
//  for (auto& [c, _] : cmap_table_.glyph_index_map) {
//    output.push_back(c);
//  }
  return output;
}

unsigned int TrueType::GetNumGlyphs() const {
  return glyf_table_.entries.size();
}

const TrueType::Tables& TrueType::GetTables() const {
  return tables_;
}

std::string TrueType::getString(int num_chars) const {
  std::string output;
  for (int i = 0; i < num_chars; ++i) {
    output.push_back(static_cast<char>(read<uint8>()));
  }
  return output;
}

uint32 TrueType::seek(uint32 new_position) const {
  GEMINI_REQUIRE(0 <= new_position && new_position < file_.size(), "new position invalid");
  std::swap(new_position, file_ptr_);
  return new_position;
}

uint32 TrueType::advance(int16 d_position) {
  return seek(file_ptr_ + d_position);
}

uint32 TrueType::calcCheckSum(uint32 offset, uint32 length) {
  auto store_file_ptr = seek(offset);
  uint32 sum = 0;
  auto nlongs = ((length + 3) / 4) | 0;
  while (nlongs--) {
    sum = (sum + read<uint32>() & 0xffffffff);
  }

  seek(store_file_ptr);
  return sum;
}

void TrueType::readHeadTable() {
  // https://docs.microsoft.com/en-us/typography/opentype/spec/head
  seek(tables_.at("head").offset);

  head_table_.version = read<Fixed>();
  head_table_.font_revision = read<Fixed>();
  head_table_.checksum_adjustment = read<uint32>();
  head_table_.magic_number = read<uint32>();
  GEMINI_ASSERT(head_table_.magic_number == 0x5f0f3cf5, "magic number is incorrect");
  head_table_.flags = read<uint16>();
  head_table_.units_per_em = read<uint16>();
  // Time indicated in "Number of seconds since 12:00 midnight that started January 1st 1904 in GMT/UTC time zone."
  head_table_.created = read<LONGDATETIME>();
  head_table_.modified = read<LONGDATETIME>();
  head_table_.xmin = read<FWORD>();
  head_table_.ymin = read<FWORD>();
  head_table_.xmax = read<FWORD>();
  head_table_.ymax = read<FWORD>();

  /* Mac style flags:
    Bit 0: Bold (if set to 1);
    Bit 1: Italic (if set to 1)
    Bit 2: Underline (if set to 1)
    Bit 3: Outline (if set to 1)
    Bit 4: Shadow (if set to 1)
    Bit 5: Condensed (if set to 1)
    Bit 6: Extended (if set to 1)
    Bits 7–15: Reserved (set to 0).
   */
  head_table_.mac_style = read<uint16>();
  head_table_.lowest_rec_PPEM = read<uint16>();
  head_table_.font_direction_hint = read<int16>();
  head_table_.index_to_loc_format = read<int16>();
  head_table_.glyph_data_format = read<int16>();
}

void TrueType::readMAXPTable() {
  seek(tables_.at("maxp").offset);

  maxp_table_.version = read<Fixed>();
  maxp_table_.num_glyphs = read<uint16>();
  maxp_table_.max_points = read<uint16>();
  maxp_table_.max_contours = read<uint16>();
  maxp_table_.max_composite_points = read<uint16>();
  maxp_table_.max_composite_contours = read<uint16>();
  maxp_table_.max_zones = read<uint16>();
  maxp_table_.max_twilight_points = read<uint16>();
  maxp_table_.max_storage = read<uint16>();
  maxp_table_.max_function_defs = read<uint16>();
  maxp_table_.max_instruction_defs = read<uint16>();
  maxp_table_.max_stack_elements = read<uint16>();
  maxp_table_.max_size_of_instructions = read<uint16>();
  maxp_table_.max_component_elements = read<uint16>();
  maxp_table_.max_component_depth = read<uint16>();
}

void TrueType::readHHEATable() {
  seek(tables_.at("hhea").offset);

  hhea_table_.version = read<Fixed>();
  hhea_table_.ascent = read<FWORD>();
  hhea_table_.descent = read<FWORD>();
  hhea_table_.line_gap = read<FWORD>();
  hhea_table_.advance_width_max = read<UFWORD>();
  hhea_table_.min_left_side_bearing = read<FWORD>();
  hhea_table_.min_right_side_bearings = read<FWORD>();
  hhea_table_.x_max_extent = read<FWORD>();
  hhea_table_.caret_slope_rise = read<int16>();
  hhea_table_.care_slope_run = read<int16>();
  hhea_table_.caret_offset = read<FWORD>();

  // Skip the four reserved places, which must be zero.
  for (int i = 0; i < 4; ++i) {
    auto x = read<int16>();
    GEMINI_ASSERT(x == 0, "reserved word must be zero");
  }

  hhea_table_.metric_data_format = read<int16>();
  hhea_table_.num_of_long_hor_metrics = read<uint16>();
}

void TrueType::readHMTXTable() {
  seek(tables_.at("hmtx").offset);

  for (int i = 0; i < hhea_table_.num_of_long_hor_metrics; ++i) {
    // Read advanced width and left side bearing.
    hmtx_table_.hmetrics.push_back({read<uint16>(), read<int16>()});
  }

  for (int i = 0; i < maxp_table_.num_glyphs - hhea_table_.num_of_long_hor_metrics; ++i) {
    hmtx_table_.left_side_bearings.push_back(read<int16>());
  }
}

void TrueType::readNAMETable() {
  seek(tables_.at("name").offset);

  // TODO: Finish this.

  name_table_.version = read<uint16>();
  name_table_.count = read<uint16>();
  name_table_.storage_offset = read<Offset16>();

  // Get name record.

  if (name_table_.version == 1) {
    name_table_.lang_tag_count = read<uint16>();
    // Get lang tag record.
  }
}

void TrueType::readPOSTTable() {
  seek(tables_.at("post").offset);

  // Read header
  fill(post_table_.version);
  fill(post_table_.italic_angle);
  fill(post_table_.underline_position);
  fill(post_table_.underline_thickness);
  fill(post_table_.is_fixed_pitch);
  fill(post_table_.min_mem_type_42);
  fill(post_table_.max_mem_type_42);
  fill(post_table_.min_mem_type_1);
  fill(post_table_.max_mem_type_1);
}

void TrueType::readLOCATable() {
  seek(tables_.at("loca").offset);
  bool is_16 = head_table_.index_to_loc_format == 0;
  // There are num_glyphs + 1 entries so it is possible to calculate the width of the last glyph.
  for (int i = 0; i <= maxp_table_.num_glyphs; ++i) {
    uint32 x = is_16 ? 2 * static_cast<uint32>(read<uint16>()) : read<uint32>();
    loca_table_.entries.push_back(x);
  }
}

void TrueType::readGLYFTable() {
  auto table_offset = tables_.at("glyf").offset;
  for (int i = 0; i < loca_table_.entries.size() - 1; ++i) {
    auto offset = loca_table_.entries[i];

    // From https://docs.microsoft.com/en-us/typography/opentype/spec/loca:
    //    "If the font does not contain an outline for the missing character, then the first and second offsets should have
    //    the same value. This also applies to any other characters without an outline, such as the space character.
    //    If a glyph has no outline, then loca[n] = loca [n+1]."
    if (offset == loca_table_.entries[i + 1]) {
      glyf_table_.entries.emplace_back(
          GlyphData{ 0, 0, 0, 0, 0});
      continue;
    }

    seek(table_offset + offset);

    // Read the glyph header and create an entry for this glyph.
    glyf_table_.entries.emplace_back(
        GlyphData{
            read<int16>(),
            read<int16>(),
            read<int16>(),
            read<int16>(),
            read<int16>()});
    auto& glyph = glyf_table_.entries.back();
    glyph.type = glyph.number_of_contours != -1 ? GlyphType::Simple : GlyphType::Compound;

    // Simple glyphs.
    if (glyph.type == GlyphType::Simple) {
      // Read the simple glyph table.

      for (int c = 0; c < glyph.number_of_contours; ++c) {
        glyph.spline.contour_ends.push_back(read<uint16>());
      }
      auto instruction_length = read<uint16>();
      for (int inst = 0; inst < instruction_length; ++inst) {
        glyph.instructions_.push_back(read<uint8>());
      }

      // Get flags.
      constexpr uint8 ON_CURVE_POINT                       = 0b00000001;
      constexpr uint8 X_SHORT_VECTOR                       = 0b00000010;
      constexpr uint8 Y_SHORT_VECTOR                       = 0b00000100;
      constexpr uint8 REPEAT_FLAG                          = 0b00001000;
      constexpr uint8 X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR = 0b00010000;
      constexpr uint8 Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR = 0b00100000;
      constexpr uint8 OVERLAP_SIMPLE                       = 0b01000000;
      constexpr uint8 RESERVED                             = 0b10000000;

      auto num_points = *std::max_element(glyph.spline.contour_ends.begin(), glyph.spline.contour_ends.end()) + 1;

      std::vector<uint8> flags;

      // Fill the array of flags.
      int repeat_count = 0;
      uint8 flag;
      for (int n = 0; n < num_points; ++n) {
        if (0 < repeat_count) {
          --repeat_count;
        }
        else {
          flag = read<uint8>();
          // Check to make sure reserved bit is zero.
          GEMINI_ASSERT((RESERVED & flag) == 0, "reserved bit is not zero");
          if (REPEAT_FLAG & flag) {
            repeat_count = read<uint8>();
          }
        }

        flags.push_back(flag);
        // Create an entry for the point, set the is_on_curve data now.
        glyph.spline.points.push_back({ 0, 0, static_cast<bool>(ON_CURVE_POINT & flag) });

        // This should be set on the first entry if it is used.
        if (OVERLAP_SIMPLE & flag) {
          glyph.simple_overlap = true;
        }
      }

      // Get all x coordinates.
      for (int n = 0; n < num_points; ++n) {
        flag = flags[n];
        bool x_is_short = X_SHORT_VECTOR & flag;
        bool x_special = X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR & flag;

        // Get (relative) X coordinate.
        int16 dx;
        if (x_is_short) {
          dx = static_cast<int16>(static_cast<int16>(read<uint8>()) * (x_special ? +1 : -1));
        }
        else {
          if (!x_special) { // Two byte signed int for the x coord.
            dx = read<int16>();
          }
          else { // Repeated x point.
            dx = 0;
          }
        }

        glyph.spline.points[n].x = (n == 0) ? dx : glyph.spline.points[n - 1].x + dx;
      }

      // Get all y coordinates.
      for (int n = 0; n < num_points; ++n) {
        flag = flags[n];
        bool y_is_short = Y_SHORT_VECTOR & flag;
        bool y_special = Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR & flag;

        // Get (relative) Y coordinate.
        int16 dy;
        if (y_is_short) {
          dy = static_cast<int16>(static_cast<int16>(read<uint8>()) * (y_special ? +1 : -1));
        }
        else {
          if (!y_special) { // Two byte signed int for the y coord.
            dy = read<int16>();
          }
          else { // Repeated y point.
            dy = 0;
          }
        }

        glyph.spline.points[n].y = (n == 0) ? dy : glyph.spline.points[n - 1].y + dy;
      }

    }
    // Complex glyphs.
    else {
      // Not handling complex glyphs right now.
    }
  }

}

//! \brief Read the CMAP table.
//!
//! See documentation: https://docs.microsoft.com/en-us/typography/opentype/spec/cmap.
void TrueType::readCMAPTable() {
  seek(tables_.at("cmap").offset);

  cmap_table_.version = read<uint16>();
  cmap_table_.num_tables = read<uint16>();

  // Right now, set up only for version 0.
  GEMINI_REQUIRE(cmap_table_.version == 0, "unsupported version: " << cmap_table_.version);

  for (int i = 0; i < cmap_table_.num_tables; ++i) {
    cmap_table_.encoding_records.push_back(
        CMAPEncoding{read<uint16>(), read<uint16>(), read<uint32>()});
  }

  // TODO: Support more formats?

  // Platform IDs
  // 0 - Unicode
  // 1 - Maintosh
  // 2 - ISO (deprecated)
  // 3 - Windows
  // 4 - Custom.

  auto base_offset = tables_.at("cmap").offset;
  for (auto& [platform_id, encoding_id, offset] : cmap_table_.encoding_records) {

    bool is_windows = platform_id == 3 && (encoding_id == 0 || encoding_id == 1 || encoding_id == 10);
    bool is_unicode = platform_id == 0 && (0 <= encoding_id && encoding_id < 5);

    // Read the version.
    seek(base_offset + offset);
    cmap_table_.format = read<uint16>();

    // There are seven possible formats.
    cmap_table_.glyph_index_map.emplace_back();
    switch (cmap_table_.format) {
      case 4:
        cmap_table_.glyph_index_map.back() = parseFormat4().glyph_index_map;
        break;
      case 14:
        parseFormat14();
        break;
      default:
        ;
        //GEMINI_FAIL("unsupported CMAP table format: " << cmap_table_.format);
    }
  }
}

//! \brief Get all the glyph spacing information.
void TrueType::getSpacingInformation() {
  // Spacing information.
  for (int index = 0; index < glyf_table_.entries.size(); ++index) {
    auto glyf = glyf_table_.entries[index];
    auto hmtx = hmtx_table_.hmetrics[index];

    auto width = static_cast<uint16>(glyf.xmax - glyf.xmin);
    auto height = static_cast<uint16>(glyf.ymax - glyf.ymin);
    auto rsb = static_cast<int16>(hmtx.advance_width - hmtx.left_side_bearing - width);
    spacing_map_[index] = SpacingInfo{glyf.xmin,
                                      glyf.ymin,
                                      width,
                                      height,
                                      hmtx.left_side_bearing,
                                      rsb,
                                      hmtx.advance_width};
  }
}

TrueType::Format4Data TrueType::parseFormat4() {
  Format4Data format;
  format.length = read<uint16>();
  format.language = read<uint16>();
  format.seg_count = read<uint16>();
  format.search_range = read<uint16>();
  format.entry_selector = read<uint16>();
  format.range_shift = read<uint16>();

  // Note that (per the spec!) what is actually stored is the segment count x 2.
  format.seg_count >>= 1;

  GEMINI_ASSERT(format.search_range == 2 * std::pow(2, floor(std::log2(format.seg_count))),
                "invalid search range");
  GEMINI_ASSERT(format.entry_selector == std::log2(format.search_range / 2), "invalid entry selector");
  GEMINI_ASSERT(format.range_shift == 2 * format.seg_count - format.search_range, "invalid range shift");

  for (auto i = 0; i < format.seg_count; ++i) {
    format.end_code.push_back(read<uint16>());
  }

  // Reserved padding. Set to zero (0x0000).
  auto pad = read<uint16>(); //
  GEMINI_ASSERT(pad == 0, "Reserved pad should be zero");

  for (auto i = 0; i < format.seg_count; ++i) {
    format.start_code.push_back(read<int16>());
  }
  for (auto i = 0; i < format.seg_count; ++i) {
    format.id_delta.push_back(read<int16>());
  }
  auto id_range_offset_start = file_ptr_;
  for (auto i = 0; i < format.seg_count; ++i) {
    format.id_range_offset.push_back(read<uint16>());
  }

  // Create a glyph index map: Unicode -> glyf index.
  // The last entry in the table is just a relic that allowed for efficient binary search and
  // consists of start/end entries of 0xFFFF. It is not a real character so we skip it.

  GEMINI_ASSERT(format.start_code.back() == 0xFFFF, "last start code entry expected to be 0xFFFF");
  GEMINI_ASSERT(format.end_code.back() == 0xFFFF, "last end code entry expected to be 0xFFFF");

  auto count_chars = 0;
  for (auto i = 0; i < format.seg_count - 1; ++i) {
    uint16 glyph_index = 0;
    auto start_code = format.start_code[i];
    auto end_code = format.end_code[i];
    auto id_delta = format.id_delta[i];
    auto id_range_offset = format.id_range_offset[i];

    for (auto c = start_code; c <= end_code; ++c) {
      if (id_range_offset == 0) {
        glyph_index = (c + id_delta) & 0xFFFF;
      }
      else {
        auto address = id_range_offset_start + 2 * i;
        uint32 location = id_range_offset + 2 * (c - start_code) + address;
        glyph_index = read_at<uint16>(location);
      }

      format.glyph_index_map[c] = glyph_index;
      ++count_chars;
    }
  }

  return format;
}



struct UnicodeRange {
  //! \brief First unicode value in this range.
  uint32 start_unicode_value{}; // uint24
  //! \brief Number of additional values in this range.
  uint8 additional_count{};
};

struct DefaultUVSTable {
  std::vector<UnicodeRange> ranges;
};

struct UVSMappingRecord {
  uint32 unicode_value{}; // uint24
  uint16 glyph_id{};
};

//! \brief A list of pairs of unicode scalar values and glyph ids.
struct NonDefaultUVSTable {
  std::vector<UVSMappingRecord> uvs_mappings;
};

struct VariationSelector {
  uint32 var_selector; // uint24
  //! \brief Offset from the start of the format 14 subtable to Default UVS Table. May be 0.
  Offset32 default_uvs_offset;
  //! \brief Offset from the start of the format 14 subtable to Non-Default UVS Table. May be 0.
  Offset32 non_default_uvs_offset;

  DefaultUVSTable default_uvs_table;

  NonDefaultUVSTable non_default_uvs_table;
};

struct Format14Data {
  uint32 length{};

  //! "The Variation Selector Records are sorted in increasing order of varSelector. No two records may have the same varSelector."
  std::vector<VariationSelector> var_selectors;
};

void TrueType::parseFormat14() {
  // Mark the start of the format 14 subtable. Note - the version of the table has already been parsed.
  auto start_ptr = file_ptr_ - 2;

  Format14Data format;

  fill(format.length);
  auto num_var_selector_records = read<uint32>();

  for (uint32 i = 0; i < num_var_selector_records; ++i) {
    // Parse the variation selector
    VariationSelector variation_selector{};
    read<1, uint32>(variation_selector.var_selector);  // uint24
    fill(variation_selector.default_uvs_offset);
    fill(variation_selector.non_default_uvs_offset);

    format.var_selectors.push_back(variation_selector);
  }

  // Parse the tables.
  for (auto& variation_selector : format.var_selectors) {
    // Default UVS table.
    seek(start_ptr + variation_selector.default_uvs_offset);
    auto num_unicode_value_ranges = read<uint32>();
    for (uint32 i = 0; i < num_unicode_value_ranges; ++i) {
      variation_selector.default_uvs_table.ranges.emplace_back();
      auto& entry = variation_selector.default_uvs_table.ranges.back();
      read<1, uint32>(entry.start_unicode_value); // uint24
      fill(entry.additional_count);
    }

    // Non-default UVS table.
    seek(start_ptr + variation_selector.non_default_uvs_offset);
    auto num_uvs_mappings = read<uint32>();
    for (uint32 i = 0; i < num_uvs_mappings; ++i) {
      variation_selector.non_default_uvs_table.uvs_mappings.emplace_back();
      auto& entry = variation_selector.non_default_uvs_table.uvs_mappings.back();
      read<1, uint32>(entry.unicode_value); // uint24
      fill(entry.glyph_id);
    }
  }
}
