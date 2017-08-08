#include "Filter.hpp"
#include "../Visitor.hpp"


using namespace svgdom;

void FilterElement::accept(Visitor& visitor) const {
	visitor.visit(*this);
}

void FeGaussianBlurElement::accept(Visitor& visitor) const {
	visitor.visit(*this);
}
