#include "parser.hxx"
#include "util.hxx"
#include "malformed_svg_error.hpp"
#include "util/casters.hpp"

#include <utki/debug.hpp>
#include <utki/util.hpp>
#include <utki/string.hpp>

#include <papki/span_file.hpp>

using namespace svgdom;

namespace{
const std::string DSvgNamespace = "http://www.w3.org/2000/svg";
const std::string DXlinkNamespace = "http://www.w3.org/1999/xlink";
}

namespace{
gradient::spread_method gradientStringToSpreadMethod(const std::string& str){
	if(str == "pad"){
		return gradient::spread_method::pad;
	}else if(str == "reflect"){
		return gradient::spread_method::reflect;
	}else if(str == "repeat"){
		return gradient::spread_method::repeat;
	}
	return gradient::spread_method::default_;
}
}

void parser::push_namespaces(){
	// parse default namespace
	{
		auto i = this->attributes.find("xmlns");
		if(i != this->attributes.end()){
			if(i->second == DSvgNamespace){
				this->default_namespace_stack.push_back(xml_namespace::svg);
			}else if(i->second == DXlinkNamespace){
				this->default_namespace_stack.push_back(xml_namespace::xlink);
			}else{
				this->default_namespace_stack.push_back(xml_namespace::unknown);
			}
		}else{
			if(this->default_namespace_stack.size() == 0){
				this->default_namespace_stack.push_back(xml_namespace::unknown);
			}else{
				this->default_namespace_stack.push_back(this->default_namespace_stack.back());
			}
		}
	}
	
	//parse other namespaces
	{
		std::string xmlns = "xmlns:";
		
		this->namespace_stack.push_back(decltype(this->namespace_stack)::value_type());
		
		for(auto& e : this->attributes){
			const auto& attr = e.first;
			
			if(attr.substr(0, xmlns.length()) != xmlns){
				continue;
			}
			
			ASSERT(attr.length() >= xmlns.length())
			auto nsName = attr.substr(xmlns.length(), attr.length() - xmlns.length());
			
			if(e.second == DSvgNamespace){
				this->namespace_stack.back()[nsName] = xml_namespace::svg;
			}else if(e.second == DXlinkNamespace){
				this->namespace_stack.back()[nsName] = xml_namespace::xlink;
			}
		}
		
		this->flipped_namespace_stack.push_back(utki::flip_map(this->namespace_stack.back()));
	}
}

void parser::pop_namespaces(){
	ASSERT(this->namespace_stack.size() != 0)
	this->namespace_stack.pop_back();
	ASSERT(this->default_namespace_stack.size() != 0)
	this->default_namespace_stack.pop_back();
	ASSERT(this->flipped_namespace_stack.size() != 0)
	this->flipped_namespace_stack.pop_back();
}

void parser::parse_element(){
	auto nsn = this->get_namespace(this->cur_element);
	// TRACE(<< "nsn.name = " << nsn.name << std::endl)
	switch(nsn.ns){
		case xml_namespace::svg:
			if(nsn.name == svg_element::tag){
				this->parseSvgElement();
			}else if(nsn.name == symbol_element::tag){
				this->parseSymbolElement();
			}else if(nsn.name == g_element::tag){
				this->parseGElement();
			}else if(nsn.name == defs_element::tag){
				this->parseDefsElement();
			}else if(nsn.name == use_element::tag){
				this->parseUseElement();
			}else if(nsn.name == path_element::tag){
				this->parsePathElement();
			}else if(nsn.name == linear_gradient_element::tag){
				this->parseLinearGradientElement();
			}else if(nsn.name == radial_gradient_element::tag){
				this->parseRadialGradientElement();
			}else if(nsn.name == gradient::stop_element::tag){
				this->parseGradientStopElement();
			}else if(nsn.name == rect_element::tag){
				this->parseRectElement();
			}else if(nsn.name == circle_element::tag){
				this->parseCircleElement();
			}else if(nsn.name == ellipse_element::tag){
				this->parseEllipseElement();
			}else if(nsn.name == line_element::tag){
				this->parseLineElement();
			}else if(nsn.name == polyline_element::tag){
				this->parsePolylineElement();
			}else if(nsn.name == polygon_element::tag){
				this->parsePolygonElement();
			}else if(nsn.name == filter_element::tag){
				this->parseFilterElement();
			}else if(nsn.name == fe_gaussian_blur_element::tag){
				this->parseFeGaussianBlurElement();
			}else if(nsn.name == fe_color_matrix_element::tag){
				this->parseFeColorMatrixElement();
			}else if(nsn.name == fe_blend_element::tag){
				this->parseFeBlendElement();
			}else if(nsn.name == fe_composite_element::tag){
				this->parseFeCompositeElement();
			}else if(nsn.name == image_element::tag){
				this->parseImageElement();
			}else if(nsn.name == mask_element::tag){
				this->parseMaskElement();
			}else if(nsn.name == text_element::tag){
				this->parseTextElement();
			}else if(nsn.name == style_element::tag){
				this->parse_style_element();
			}else{
				// unknown element, ignore
				break;
			}
			return;
		default:
			// unknown namespace, ignore
			break;
	}
	this->element_stack.push_back(nullptr);
}

parser::xml_namespace parser::find_namespace(const std::string& ns){
	for(auto i = this->namespace_stack.rbegin(), e = this->namespace_stack.rend(); i != e; ++i){
		auto iter = i->find(ns);
		if(iter == i->end()){
			continue;
		}
		ASSERT(ns == iter->first)
		return iter->second;
	}
	return xml_namespace::unknown;
}

const std::string* parser::find_flipped_namespace(xml_namespace ns){
	for(auto i = this->flipped_namespace_stack.rbegin(), e = this->flipped_namespace_stack.rend(); i != e; ++i){
		auto iter = i->find(ns);
		if(iter == i->end()){
			continue;
		}
		ASSERT(ns == iter->first)
		return &iter->second;
	}
	return nullptr;
}

parser::namespace_name_pair parser::get_namespace(const std::string& xmlName){
	namespace_name_pair ret;

	auto colonIndex = xmlName.find_first_of(':');
	if(colonIndex == std::string::npos){
		ret.ns = this->default_namespace_stack.back();
		ret.name = xmlName;
		return ret;
	}

	ASSERT(xmlName.length() >= colonIndex + 1)

	ret.ns = this->find_namespace(xmlName.substr(0, colonIndex));
	ret.name = xmlName.substr(colonIndex + 1, xmlName.length() - 1 - colonIndex);

	return ret;
}

const std::string* parser::find_attribute(const std::string& name){
	auto i = this->attributes.find(name);
	if(i != this->attributes.end()){
		return &i->second;
	}
	return nullptr;
}

const std::string* parser::find_attribute_of_namespace(xml_namespace ns, const std::string& name){
	if(this->default_namespace_stack.back() == ns){
		if(auto a = this->find_attribute(name)){
			return a;
		}
	}
	
	if(auto prefix = this->find_flipped_namespace(ns)){
		if(auto a = this->find_attribute(*prefix + ":" + name)){
			return a;
		}
	}
	return nullptr;
}

void parser::fill_element(element& e){
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "id")){
		e.id = *a;
	}
}

void parser::fillGradient(gradient& g){
	this->fill_element(g);
	this->fill_referencing(g);
	this->fillStyleable(g);

	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "spreadMethod")){
		g.spread_method_ = gradientStringToSpreadMethod(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "gradientTransform")){
		g.transformations = transformable::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "gradientUnits")){
		g.units = parse_coordinate_units(*a);
	}
}

void parser::fill_rectangle(rectangle& r, const rectangle& defaultValues){
	r = defaultValues;
	
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "x")){
		r.x = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "y")){
		r.y = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "width")){
		r.width = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "height")){
		r.height = length::parse(*a);
	}
}

void parser::fill_referencing(referencing& e){
	auto a = this->find_attribute_of_namespace(xml_namespace::xlink, "href");
	if(!a){
		a = this->find_attribute_of_namespace(xml_namespace::svg, "href");//in some SVG documents the svg namespace is used instead of xlink, though this is against SVG spec we allow to do so.
	}
	if(a){
		e.iri = *a;
	}
}

void parser::fillShape(shape& s){
	this->fill_element(s);
	this->fillStyleable(s);
	this->fillTransformable(s);
}

void parser::fillStyleable(styleable& s){
	ASSERT(s.styles.size() == 0)

	for(auto& a : this->attributes){
		auto nsn = this->get_namespace(a.first);
		switch (nsn.ns){
			case xml_namespace::svg:
				if(nsn.name == "style"){
					s.styles = styleable::parse(a.second);
					break;
				}else if(nsn.name == "class"){
					s.classes = utki::split(a.second);
					break;
				}

				// parse style attributes
				{
					style_property type = styleable::string_to_property(nsn.name);
					if(type != style_property::unknown){
						s.presentation_attributes[type] = styleable::parse_style_property_value(type, a.second);
					}
				}
				break;
			default:
				break;
		}
	}
}

void parser::fillTransformable(transformable& t){
	ASSERT(t.transformations.size() == 0)
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "transform")){
		t.transformations = transformable::parse(*a);
	}
}

void parser::fillViewBoxed(view_boxed& v){
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "viewBox")){
		v.view_box = svg_element::parse_view_box(*a);
	}
}

void parser::fillTextPositioning(text_positioning& p){
	// TODO: parse missing attributes
}

void parser::fill_style(style_element& e){
	// TODO: parse missing attributes
}

void parser::fillAspectRatioed(aspect_ratioed& e){
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "preserveAspectRatio")){
		e.preserve_aspect_ratio.parse(*a);
	}
}

void parser::add_element(std::unique_ptr<element> e){
	ASSERT(e)
	
	auto elem = e.get();

	if(this->element_stack.empty()){
		if(this->svg){
			throw malformed_svg_error("more than one root element found in the SVG document");
		}

		element_caster<svg_element> c;
		e->accept(c);
		if(!c.pointer){
			throw malformed_svg_error("first element of the SVG document is not an 'svg' element");
		}
		
		e.release();
		this->svg = decltype(this->svg)(c.pointer);
	}else{
		container_caster c;
		auto parent = this->element_stack.back();
		if(parent){
			parent->accept(c);
		}
		if(c.pointer){
			c.pointer->children.push_back(std::move(e));
		}else{
			elem = nullptr;
		}
	}
	this->element_stack.push_back(elem);
}

void parser::parseCircleElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == circle_element::tag)

	auto ret = std::make_unique<circle_element>();

	this->fillShape(*ret);

	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "cx")){
		ret->cx = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "cy")){
		ret->cy = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "r")){
		ret->r = length::parse(*a);
	}

	this->add_element(std::move(ret));
}

void parser::parseDefsElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == defs_element::tag)

	auto ret = std::make_unique<defs_element>();

	this->fill_element(*ret);
	this->fillTransformable(*ret);
	this->fillStyleable(*ret);

	this->add_element(std::move(ret));
}

void parser::parseMaskElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == mask_element::tag)

	auto ret = std::make_unique<mask_element>();

	this->fill_element(*ret);
	this->fill_rectangle(*ret);
	this->fillStyleable(*ret);

	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "maskUnits")){
		ret->mask_units = parse_coordinate_units(*a);
	}
	
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "maskContentUnits")){
		ret->mask_content_units = parse_coordinate_units(*a);
	}
	
	this->add_element(std::move(ret));
}

void parser::parseTextElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == text_element::tag)

	auto ret = std::make_unique<text_element>();

	this->fill_element(*ret);
	this->fillStyleable(*ret);
	this->fillTransformable(*ret);
	this->fillTextPositioning(*ret);
	
	//TODO: parse missing text element attributes
	
	this->add_element(std::move(ret));
}

void parser::parse_style_element(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == style_element::tag)

	auto ret = std::make_unique<style_element>();

	this->fill_element(*ret);
	this->fill_style(*ret);
	
	// TODO: parse missing style element attributes
	
	this->add_element(std::move(ret));
}

void parser::parseEllipseElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == ellipse_element::tag)

	auto ret = std::make_unique<ellipse_element>();

	this->fillShape(*ret);

	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "cx")){
		ret->cx = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "cy")){
		ret->cy = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "rx")){
		ret->rx = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "ry")){
		ret->ry = length::parse(*a);
	}

	this->add_element(std::move(ret));
}

void parser::parseGElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == g_element::tag)

	auto ret = std::make_unique<g_element>();

	this->fill_element(*ret);
	this->fillTransformable(*ret);
	this->fillStyleable(*ret);

	this->add_element(std::move(ret));
}

void parser::parseGradientStopElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == gradient::stop_element::tag)

	auto ret = std::make_unique<gradient::stop_element>();
	
	this->fillStyleable(*ret);
	
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "offset")){
		std::istringstream s(*a);
		s >> ret->offset;
		if(!s.eof() && s.peek() == '%'){
			ret->offset /= 100;
		}
	}

	this->add_element(std::move(ret));
}

void parser::parseLineElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == line_element::tag)

	auto ret = std::make_unique<line_element>();

	this->fillShape(*ret);
	
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "x1")){
		ret->x1 = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "y1")){
		ret->y1 = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "x2")){
		ret->x2 = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "y2")){
		ret->y2 = length::parse(*a);
	}


	this->add_element(std::move(ret));
}

void parser::parseFilterElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == filter_element::tag)
	
	auto ret = std::make_unique<filter_element>();
	
	this->fill_element(*ret);
	this->fillStyleable(*ret);
	this->fill_rectangle(
			*ret,
			rectangle(
					length(-10, length_unit::percent),
					length(-10, length_unit::percent),
					length(120, length_unit::percent),
					length(120, length_unit::percent)
				)
		);
	this->fill_referencing(*ret);
	
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "filterUnits")){
		ret->filter_units = svgdom::parse_coordinate_units(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "primitiveUnits")){
		ret->primitive_units = svgdom::parse_coordinate_units(*a);
	}
	
	this->add_element(std::move(ret));
}

void parser::fillFilterPrimitive(filter_primitive& p){
	this->fill_element(p);
	this->fill_rectangle(p);
	this->fillStyleable(p);

	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "result")){
		p.result = *a;
	}
}

void parser::fillInputable(inputable& p){
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "in")){
		p.in = *a;
	}
}

void parser::fillSecondInputable(second_inputable& p){
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "in2")){
		p.in2 = *a;
	}
}

void parser::parseFeGaussianBlurElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == fe_gaussian_blur_element::tag)
	
	auto ret = std::make_unique<fe_gaussian_blur_element>();
	
	this->fillFilterPrimitive(*ret);
	this->fillInputable(*ret);

	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "stdDeviation")){
		ret->std_deviation = parse_number_and_optional_number(*a, {-1, -1});
	}
	
	this->add_element(std::move(ret));
}

void parser::parseFeColorMatrixElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == fe_color_matrix_element::tag)
	
	auto ret = std::make_unique<fe_color_matrix_element>();
	
	this->fillFilterPrimitive(*ret);
	this->fillInputable(*ret);
	
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "type")){
		if(*a == "saturate"){
			ret->type_ = fe_color_matrix_element::type::saturate;
		}else if(*a == "hueRotate"){
			ret->type_ = fe_color_matrix_element::type::hue_rotate;
		}else if(*a == "luminanceToAlpha"){
			ret->type_ = fe_color_matrix_element::type::luminance_to_alpha;
		}else{
			ret->type_ = fe_color_matrix_element::type::matrix; // default value
		}
	}
	
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "values")){
		switch(ret->type_){
			default:
				ASSERT(false) // should never get here, MATRIX should always be the default value
				break;
			case fe_color_matrix_element::type::matrix:
				// 20 values expected
				{
					std::istringstream ss(*a);
					
					for(unsigned i = 0; i != 20; ++i){
						ret->values[i] = read_in_real(ss);
						if(ss.fail()){
							throw malformed_svg_error("malformed 'values' string of 'feColorMatrix' element");
						}
						skip_whitespaces_and_comma(ss);
					}
				}
				break;
			case fe_color_matrix_element::type::hue_rotate:
				// fall-through
			case fe_color_matrix_element::type::saturate:
				// one value is expected
				{
					std::istringstream ss(*a);
					ret->values[0] = read_in_real(ss);
					if(ss.fail()){
						throw malformed_svg_error("malformed 'values' string of 'feColorMatrix' element");
					}
				}
				break;
			case fe_color_matrix_element::type::luminance_to_alpha:
				// no values are expected
				break;
		}
	}
	
	this->add_element(std::move(ret));
}

void parser::parseFeBlendElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == fe_blend_element::tag)
	
	auto ret = std::make_unique<fe_blend_element>();
	
	this->fillFilterPrimitive(*ret);
	this->fillInputable(*ret);
	this->fillSecondInputable(*ret);
	
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "mode")){
		if(*a == "normal"){
			ret->mode_ = fe_blend_element::mode::normal;
		}else if(*a == "multiply"){
			ret->mode_ = fe_blend_element::mode::multiply;
		}else if(*a == "screen"){
			ret->mode_ = fe_blend_element::mode::screen;
		}else if(*a == "darken"){
			ret->mode_ = fe_blend_element::mode::darken;
		}else if(*a == "lighten"){
			ret->mode_ = fe_blend_element::mode::lighten;
		}
	}
	
	this->add_element(std::move(ret));
}

void parser::parseFeCompositeElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == fe_composite_element::tag)
	
	auto ret = std::make_unique<fe_composite_element>();
	
	this->fillFilterPrimitive(*ret);
	this->fillInputable(*ret);
	this->fillSecondInputable(*ret);
	
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "operator")){
		if(*a == "over"){
			ret->operator__ = fe_composite_element::operator_::over;
		}else if(*a == "in"){
			ret->operator__ = fe_composite_element::operator_::in;
		}else if(*a == "out"){
			ret->operator__ = fe_composite_element::operator_::out;
		}else if(*a == "atop"){
			ret->operator__ = fe_composite_element::operator_::atop;
		}else if(*a == "xor"){
			ret->operator__ = fe_composite_element::operator_::xor_;
		}else if(*a == "arithmetic"){
			ret->operator__ = fe_composite_element::operator_::arithmetic;
		}
	}
	
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "k1")){
		ret->k1 = real(std::strtod(a->c_str(), nullptr));
	}
	
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "k2")){
		ret->k2 = real(std::strtod(a->c_str(), nullptr));
	}
	
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "k3")){
		ret->k3 = real(std::strtod(a->c_str(), nullptr));
	}
	
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "k4")){
		ret->k4 = real(std::strtod(a->c_str(), nullptr));
	}
	
	this->add_element(std::move(ret));
}

void parser::parseLinearGradientElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == linear_gradient_element::tag)

	auto ret = std::make_unique<linear_gradient_element>();

	this->fillGradient(*ret);

	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "x1")){
		ret->x1 = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "y1")){
		ret->y1 = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "x2")){
		ret->x2 = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "y2")){
		ret->y2 = length::parse(*a);
	}

	this->add_element(std::move(ret));
}

void parser::parsePathElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == path_element::tag)

	auto ret = std::make_unique<path_element>();

	this->fillShape(*ret);

	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "d")){
		ret->path = path_element::parse(*a);
	}
	
	this->add_element(std::move(ret));
}

void parser::parsePolygonElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == polygon_element::tag)

	auto ret = std::make_unique<polygon_element>();

	this->fillShape(*ret);

	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "points")){
		ret->points = ret->parse(*a);
	}
	
	this->add_element(std::move(ret));
}

void parser::parsePolylineElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == polyline_element::tag)

	auto ret = std::make_unique<polyline_element>();

	this->fillShape(*ret);

	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "points")){
		ret->points = ret->parse(*a);
	}
	
	this->add_element(std::move(ret));
}

void parser::parseRadialGradientElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == radial_gradient_element::tag)

	auto ret = std::make_unique<radial_gradient_element>();

	this->fillGradient(*ret);

	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "cx")){
		ret->cx = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "cy")){
		ret->cy = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "r")){
		ret->r = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "fx")){
		ret->fx = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "fy")){
		ret->fy = length::parse(*a);
	}

	this->add_element(std::move(ret));
}

void parser::parseRectElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == rect_element::tag)

	auto ret = std::make_unique<rect_element>();

	this->fillShape(*ret);
	this->fill_rectangle(*ret, rect_element::rectangle_default_values());

	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "rx")){
		ret->rx = length::parse(*a);
	}
	if(auto a = this->find_attribute_of_namespace(xml_namespace::svg, "ry")){
		ret->ry = length::parse(*a);
	}

	this->add_element(std::move(ret));
}

void parser::parseSvgElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == svg_element::tag)

	auto ret = std::make_unique<svg_element>();

	this->fill_element(*ret);
	this->fillStyleable(*ret);
	this->fill_rectangle(*ret);
	this->fillViewBoxed(*ret);
	this->fillAspectRatioed(*ret);
	
	this->add_element(std::move(ret));
}

void parser::parseImageElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == image_element::tag)

	auto ret = std::make_unique<image_element>();

	this->fill_element(*ret);
	this->fillStyleable(*ret);
	this->fillTransformable(*ret);
	this->fill_rectangle(*ret);
	this->fill_referencing(*ret);
	this->fillAspectRatioed(*ret);

	this->add_element(std::move(ret));
}

void parser::parseSymbolElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == symbol_element::tag)

	//		TRACE(<< "parseSymbolElement():" << std::endl)

	auto ret = std::make_unique<symbol_element>();

	this->fill_element(*ret);
	this->fillStyleable(*ret);
	this->fillViewBoxed(*ret);
	this->fillAspectRatioed(*ret);

	this->add_element(std::move(ret));
}

void parser::parseUseElement(){
	ASSERT(this->get_namespace(this->cur_element).ns == xml_namespace::svg)
	ASSERT(this->get_namespace(this->cur_element).name == use_element::tag)

	auto ret = std::make_unique<use_element>();

	this->fill_element(*ret);
	this->fillTransformable(*ret);
	this->fillStyleable(*ret);
	this->fill_referencing(*ret);
	this->fill_rectangle(*ret);

	this->add_element(std::move(ret));
}

void parser::on_element_start(utki::span<const char> name){
	this->cur_element = utki::make_string(name);
}

void parser::on_element_end(utki::span<const char> name){
	this->pop_namespaces();
	this->element_stack.pop_back();
}

void parser::on_attribute_parsed(utki::span<const char> name, utki::span<const char> value){
	ASSERT(this->cur_element.length() != 0)
	this->attributes[utki::make_string(name)] = utki::make_string(value);
}

void parser::on_attributes_end(bool is_empty_element){
//	TRACE(<< "this->cur_element = " << this->cur_element << std::endl)
//	TRACE(<< "this->element_stack.size() = " << this->element_stack.size() << std::endl)
	this->push_namespaces();

	this->parse_element();

	this->attributes.clear();
	this->cur_element.clear();
}

namespace{
class parse_content_visitor : public visitor{
	const utki::span<const char> content;
public:
	parse_content_visitor(utki::span<const char> content) :
			content(content)
	{}

	void default_visit(element&, container&)override{
		// do nothing
	}

	void visit(style_element& e)override{
		e.css.append(cssom::read(
				papki::span_file(this->content),
				[](const std::string& name) -> uint32_t{
					return uint32_t(styleable::string_to_property(name));
				},
				[](uint32_t id, std::string&& v) -> std::unique_ptr<cssom::property_value_base>{
					auto sp = style_property(id);
					if(sp == style_property::unknown){
						return nullptr;
					}
					auto ret = std::make_unique<style_element::css_style_value>();
					ret->value = styleable::parse_style_property_value(sp, v);
					return ret;
				}
			));
	}
};
}

void parser::on_content_parsed(utki::span<const char> str){
	if(this->element_stack.empty() || !this->element_stack.back()){
		return;
	}

	parse_content_visitor v(str);
	this->element_stack.back()->accept(v);
}

std::unique_ptr<svg_element> parser::get_dom(){
	return std::move(this->svg);
}
