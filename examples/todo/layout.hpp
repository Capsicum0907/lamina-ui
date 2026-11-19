#pragma once

#include <optional>
#include <vector>

namespace lamina::examples::todo {

struct Box {
    double x;
    double y;
    double width;
    double height;
};

struct LayoutSettings {
    double margin = 16.0;
    double max_column_width = 640.0;
    double field_height = 56.0;
    double add_size = 56.0;
    double add_gap = 8.0;
    double list_gap = 8.0;
    double list_top_padding = 4.0;
    double list_end_padding = 72.0;
    double check_size = 40.0;
    double text_gap = 12.0;
    double remove_size = 40.0;
    double remove_gap = 8.0;
    double section_gap = 8.0;
    double header_height = 48.0;
};

struct Column {
    double x;
    double width;
    double text_x;
    double text_width;
    double remove_x;
};

struct ListInput {
    std::vector<double> active;
    std::vector<double> completed;
    bool expanded = true;
};

struct RowPlace {
    double y;
    double height;
    bool shown;
};

struct TodoFrame {
    Box field;
    Box add;
    Box viewport;
    Column column;
    std::vector<RowPlace> active;
    std::vector<RowPlace> completed;
    std::optional<Box> header;
    double content_height;
};

class TodoLayout {
public:
    explicit TodoLayout(LayoutSettings settings = {});

    Column column(double width) const;
    TodoFrame place(double width, double height, const ListInput& rows) const;

    const LayoutSettings& settings() const { return settings_; }

private:
    LayoutSettings settings_;
};

}
