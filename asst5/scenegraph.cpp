#include <algorithm>

#include "scenegraph.h"

using namespace std;

bool SgTransformNode::accept(SgNodeVisitor& visitor) {
	if (!visitor.visit(*this))
		return false;
	for (int i = 0, n = children_.size(); i < n; ++i) {
		if (!children_[i]->accept(visitor))
			return false;
	}
	return visitor.postVisit(*this);
}

void SgTransformNode::addChild(shared_ptr<SgNode> child) {
	children_.push_back(child);
}

void SgTransformNode::removeChild(shared_ptr<SgNode> child) {
	children_.erase(find(children_.begin(), children_.end(), child));
}

bool SgShapeNode::accept(SgNodeVisitor& visitor) {
	if (!visitor.visit(*this))
		return false;
	return visitor.postVisit(*this);
}

class RbtAccumVisitor : public SgNodeVisitor {
protected:
	vector<RigTForm> rbtStack_;
	SgTransformNode& target_;
	bool found_;
public:
	RbtAccumVisitor(SgTransformNode& target)
		: target_(target)
		, found_(false) {}

	const RigTForm getAccumulatedRbt(int offsetFromStackTop = 0) {
		// TODO
		//cout << "get rbtStack_ length " << rbtStack_.size() << endl;
		RigTForm A;
		int cnt = offsetFromStackTop;
		if (found_) {
			//cout << "get Find!" << endl;
			while (cnt != 0) {
				rbtStack_.pop_back();
				cnt--;
			}
			while (rbtStack_.size() != 0) {
				A = rbtStack_.back() * A;
				rbtStack_.pop_back();
			}
		}
		Cvec3 t = A.getTranslation();
		//cout << "AccumRbt " << t[0]  << " " << t[1] << " " << t[2] << endl;
		return A;
	}

	virtual bool visit(SgTransformNode& node) {
		// TODO
		rbtStack_.push_back(node.getRbt());
		//cout << "visit rbtStack_ length " << rbtStack_.size() << endl;
		if (target_ == node) {
			//cout << "visit Find!" << endl;
			//cout << "node's child? " << node.getNumChildren() << endl;
			found_ = true;
			return false;
		}
		/*if (found_) {
			cout << "why don't stop visit?" << endl;
		}*/
		return true;
	}

	virtual bool postVisit(SgTransformNode& node) {
		// TODO
		rbtStack_.pop_back();
		//cout << "post rbtStack_ length " << rbtStack_.size() << endl;
		return true;
	}
};

RigTForm getPathAccumRbt(
	shared_ptr<SgTransformNode> source,
	shared_ptr<SgTransformNode> destination,
	int offsetFromDestination) {

	RbtAccumVisitor accum(*destination);
	source->accept(accum);
	return accum.getAccumulatedRbt(offsetFromDestination);
}
