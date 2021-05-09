/*
 * Copyright (c) 2020, Matthew Olsson <matthewcolsson@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGfx/Painter.h>
#include <LibWeb/Layout/SVGPathBox.h>
#include <LibWeb/SVG/SVGPathElement.h>

namespace Web::Layout {

SVGPathBox::SVGPathBox(DOM::Document& document, SVG::SVGPathElement& element, NonnullRefPtr<CSS::StyleProperties> properties)
    : SVGGraphicsBox(document, element, properties)
{
}

void SVGPathBox::prepare_for_replaced_layout()
{
    auto& bounding_box = dom_node().get_path().bounding_box();
    set_has_intrinsic_width(true);
    set_has_intrinsic_height(true);
    set_intrinsic_width(bounding_box.width());
    set_intrinsic_height(bounding_box.height());

    // FIXME: This does not belong here! Someone at a higher level should place this box.
    set_offset(bounding_box.top_left());
}

void SVGPathBox::paint(PaintContext& context, PaintPhase phase)
{
    if (!is_visible())
        return;

    SVGGraphicsBox::paint(context, phase);

    if (phase != PaintPhase::Foreground)
        return;

    auto& path_element = dom_node();
    auto& path = path_element.get_path();

    // We need to fill the path before applying the stroke, however the filled
    // path must be closed, whereas the stroke path may not necessary be closed.
    // Copy the path and close it for filling, but use the previous path for stroke
    auto closed_path = path;
    closed_path.close();

    // Fills are computed as though all paths are closed (https://svgwg.org/svg2-draft/painting.html#FillProperties)
    auto& painter = context.painter();
    auto& svg_context = context.svg_context();

    auto offset = (absolute_position() - effective_offset()).to_type<int>();

    painter.translate(offset);

    painter.fill_path(
        closed_path,
        path_element.fill_color().value_or(svg_context.fill_color()),
        Gfx::Painter::WindingRule::EvenOdd);
    painter.stroke_path(
        path,
        path_element.stroke_color().value_or(svg_context.stroke_color()),
        path_element.stroke_width().value_or(svg_context.stroke_width()));

    painter.translate(-offset);
}

}
