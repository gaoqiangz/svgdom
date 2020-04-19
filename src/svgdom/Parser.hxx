#pragma once

#include <map>
#include <vector>
#include <memory>

#include <mikroxml/mikroxml.hpp>

#include "elements/element.hpp"
#include "elements/Referencing.hpp"
#include "elements/Rectangle.hpp"
#include "elements/ViewBoxed.hpp"
#include "elements/Transformable.hpp"
#include "elements/gradients.hpp"
#include "elements/Shapes.hpp"
#include "elements/Structurals.hpp"
#include "elements/Filter.hpp"
#include "elements/image_element.hpp"
#include "elements/TextElement.hpp"

namespace svgdom{

class Parser : public mikroxml::parser{
	enum class XmlNamespace_e{
		ENUM_FIRST,
		UNKNOWN = ENUM_FIRST,
		SVG,
		XLINK,
		
		ENUM_SIZE
	};
	
	std::vector<
			std::map<std::string, XmlNamespace_e>
		> namespacesStack;
	std::vector<
			std::map<XmlNamespace_e, std::string>
		> flippedNamespacesStack;
	
	std::vector<XmlNamespace_e> defaultNamespaceStack;
	
	
	XmlNamespace_e findNamespace(const std::string& ns);
	const std::string* findFlippedNamespace(XmlNamespace_e ns);
	
	struct NamespaceNamePair{
		XmlNamespace_e ns;
		std::string name;
	};
	
	NamespaceNamePair getNamespace(const std::string& xmlName);
	
	const std::string* findAttribute(const std::string& name);
	
	const std::string* findAttributeOfNamespace(XmlNamespace_e ns, const std::string& name);

	void pushNamespaces();
	void popNamespaces();
	
	std::string element;
	std::map<std::string, std::string> attributes;
	
	SvgElement* svg = nullptr;
	Container root;
	std::vector<Container*> parentStack = {&this->root};
	
	void addElement(std::unique_ptr<Element> e);
	void addElement(std::unique_ptr<Element> e, Container* c);
	
	void on_element_start(const utki::span<char> name) override;
	void on_element_end(const utki::span<char> name) override;
	void on_attribute_parsed(const utki::span<char> name, const utki::span<char> value) override;
	void on_attributes_end(bool is_empty_element) override;
	void on_content_parsed(const utki::span<char> str) override;

	void fillElement(Element& e);
	void fillReferencing(Referencing& e);
	void fillRectangle(
			Rectangle& r,
			const Rectangle& defaultValues = Rectangle(
					Length::make(0, Length::Unit_e::PERCENT),
					Length::make(0, Length::Unit_e::PERCENT),
					Length::make(100, Length::Unit_e::PERCENT),
					Length::make(100, Length::Unit_e::PERCENT)
				)
		);
	void fillViewBoxed(ViewBoxed& v);
	void fillAspectRatioed(AspectRatioed& e);
	void fillTransformable(Transformable& t);
	void fillStyleable(Styleable& s);
	void fillGradient(Gradient& g);
	void fillShape(Shape& s);
	void fillFilterPrimitive(FilterPrimitive& p);
	void fillInputable(Inputable& p);
	void fillSecondInputable(SecondInputable& p);
	void fillTextPositioning(TextPositioning& p);
	
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
	
	void parseNode();
public:
	std::unique_ptr<SvgElement> getDom();
};

}
