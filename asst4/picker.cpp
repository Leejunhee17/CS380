#include <GL/glew.h>

#include "picker.h"

using namespace std;

Picker::Picker(const RigTForm& initialRbt, const ShaderState& curSS)
	: drawer_(initialRbt, curSS)
	, idCounter_(0)
	, srgbFrameBuffer_(!g_Gl2Compatible) {}

bool Picker::visit(SgTransformNode& node) {
	// TODO
	shared_ptr<SgNode> p = node.shared_from_this();
	nodeStack_.push_back(p);
	return drawer_.visit(node);
}

bool Picker::postVisit(SgTransformNode& node) {
	// TODO
	nodeStack_.pop_back();
	return drawer_.postVisit(node);
}

bool Picker::visit(SgShapeNode& node) {
	// TODO
	idCounter_++;
	shared_ptr<SgNode> p = nodeStack_.back();
	shared_ptr<SgRbtNode> q = dynamic_pointer_cast<SgRbtNode>(p);
	addToMap(idCounter_, q);
	Cvec3 color = idToColor(idCounter_);
	/*cout << "id " << idCounter_ << endl;
	cout << "color " << color[0] << color[1] << color[2] << endl;*/
	RigTForm A = q->getRbt();
	//cout << idCounter_ << " => " << A.getTranslation()[0] << " " << A.getTranslation()[1] << " " << A.getTranslation()[2] << endl;
	safe_glUniform3f(drawer_.getCurSS().h_uIdColor, color[0], color[1], color[2]);
	return drawer_.visit(node);
}

bool Picker::postVisit(SgShapeNode& node) {
	// TODO
	return drawer_.postVisit(node);
}

shared_ptr<SgRbtNode> Picker::getRbtNodeAtXY(int x, int y) {
	// TODO
	//cout << "x " << x << " y "  << y << endl;
	PackedPixel pixel;
	glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &pixel);
	//cout << "image " << pixel.r << pixel.r << pixel.r << endl;
	//cout << "find? " << find(colorToId(pixel)) << endl;
	//cout << colorToId(pixel) << " is picked" << endl;
	return find(colorToId(pixel));
}

//------------------
// Helper functions
//------------------
//
void Picker::addToMap(int id, shared_ptr<SgRbtNode> node) {
	idToRbtNode_[id] = node;
}

shared_ptr<SgRbtNode> Picker::find(int id) {
	IdToRbtNodeMap::iterator it = idToRbtNode_.find(id);
	if (it != idToRbtNode_.end())
		return it->second;
	else
		return shared_ptr<SgRbtNode>(); // set to null
}

// encode 2^4 = 16 IDs in each of R, G, B channel, for a total of 16^3 number of objects
static const int NBITS = 4, N = 1 << NBITS, MASK = N - 1;

Cvec3 Picker::idToColor(int id) {
	assert(id > 0 && id < N* N* N);
	Cvec3 framebufferColor = Cvec3(id & MASK, (id >> NBITS) & MASK, (id >> (NBITS + NBITS)) & MASK);
	framebufferColor = framebufferColor / N + Cvec3(0.5 / N);

	if (!srgbFrameBuffer_)
		return framebufferColor;
	else {
		// if GL3 is used, the framebuffer will be in SRGB format, and the color we supply needs to be in linear space
		Cvec3 linearColor;
		for (int i = 0; i < 3; ++i) {
			linearColor[i] = framebufferColor[i] <= 0.04045 ? framebufferColor[i] / 12.92 : pow((framebufferColor[i] + 0.055) / 1.055, 2.4);
		}
		return linearColor;
	}
}

int Picker::colorToId(const PackedPixel& p) {
	const int UNUSED_BITS = 8 - NBITS;
	int id = p.r >> UNUSED_BITS;
	id |= ((p.g >> UNUSED_BITS) << NBITS);
	id |= ((p.b >> UNUSED_BITS) << (NBITS + NBITS));
	return id;
}
