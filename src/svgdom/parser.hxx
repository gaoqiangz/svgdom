#pragma once

#include <map>
#include <vector>
#include <memory>

#include <mikroxml/mikroxml.hpp>

#include "elements/element.hpp"
#include "elements/referencing.hpp"
#include "elements/rectangle.hpp"
#include "elements/view_boxed.hpp"
#include "elements/transformable.hpp"
#include "elements/gradients.hpp"
#include "elements/shapes.hpp"
#include "elements/structurals.hpp"
#include "elements/filter.hpp"
#include "elements/image_element.hpp"
#include "elements/text_element.hpp"
#include "elements/style.hpp"

namespace svgdom{

class parser : public mikroxml::parser{
	enum class xml_namespace{
		unknown,
		svg,
		xlink,
		
		enum_size
	};
	
	std::vector<
			std::map<std::string, xml_namespace>
		> namespace_stack;
	
	std::vector<
			std::map<xml_namespace, std::string>
		> flipped_namespace_stack;
	
	std::vector<xml_namespace> default_namespace_stack;
	
	
	xml_namespace find_namespace(const std::string& ns);
	const std::string* find_flipped_namespace(xml_namespace ns);
	
	struct namespace_name_pair{
		xml_namespace ns;
		std::string name;
	};
	
	namespace_name_pair get_namespace(const std::string& xmlName);
	
	const std::string* find_attribute(const std::string& name);
	
	const std::string* find_attribute_of_namespace(xml_namespace ns, const std::string& name);

	void push_namespaces();
	void pop_namespaces();
	
	std::string cur_element;
	std::map<std::string, std::string> attributes;
	
	std::unique_ptr<svg_element> svg; // root svg element
	std::vector<element*> element_stack;
	
	void add_element(std::unique_ptr<element> e);
	
	void on_element_start(utki::span<const char> name) override;
	void on_element_end(utki::span<const char> name) override;
	void on_attribute_parsed(utki::span<const char> name, utki::span<const char> value) override;
	void on_attributes_end(bool is_empty_element) override;
	void on_content_parsed(utki::span<const char> str) override;

	void fill_element(element& e);
	void fill_referencing(referencing& e);
	void fill_rectangle(
			rectangle& r,
			const rectangle& defaultValues = rectangle(
					length(0, length_unit::percent),
					length(0, length_unit::percent),
					length(100, length_unit::percent),
					length(100, length_unit::percent)
				)
		);
	void fill_view_boxed(view_boxed& v);
	void fill_aspect_ratioed(aspect_ratioed& e);
	void fill_transformable(transformable& t);
	void fill_styleable(styleable& s);
	void fill_gradient(gradient& g);
	void fillShape(shape& s);
	void fillFilterPrimitive(filter_primitive& p);
	void fillInputable(inputable& p);
	void fillSecondInputable(second_inputable& p);
	void fillTextPositioning(text_positioning& p);
	void fill_style(style_element& e);
	
	void parseGradientStopElement();
	void parseSvgElement();
	void parseSymbolElement();
	void parseGElement();
	void parseDefsElement();
	void parseUseElement();
	void parsePathElement();
	void parseRectElement();
	void parseCircleElement();
	void parseLineElement();
	void parsePolylineElement();
	void parsePolygonElement();
	void parseEllipseElement();
	void parseLinearGradientElement();
	void parseRadialGradientElement();
	void parseFilterElement();
	void parseFeGaussianBlurElement();
	void parseFeColorMatrixElement();
	void parseFeBlendElement();
	void parseFeCompositeElement();
	void parseImageElement();
	void parseMaskElement();
	void parseTextElement();
	void parse_style_element();
	
	void parse_element();
public:
	std::unique_ptr<svg_element> get_dom();
};

}
