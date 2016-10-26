/*
  The contents of this file are dedicated by all of its authors, including

    Michael S. Gashler,
    Luke B. Godfrey,
    anonymous contributors,

  to the public domain (http://creativecommons.org/publicdomain/zero/1.0/).

  Note that some moral obligations still exist in the absence of legal ones.
  For example, it would still be dishonest to deliberately misrepresent the
  origin of a work. Although we impose no legal requirements to obtain a
  license, it is beseeming for those who build on the works of others to
  give back useful improvements, or find a way to pay it forward. If
  you would like to cite us, a published paper about Waffles can be found
  at http://jmlr.org/papers/volume12/gashler11a/gashler11a.pdf. If you find
  our code to be useful, the Waffles team would love to hear how you use it.
*/

#ifndef __GSEARCH_H__
#define __GSEARCH_H__

#include "GError.h"
#include "GMatrix.h"
#include <vector>

namespace GClasses {

class GActionPath;
class GAction;
class GRand;
class GNeuralNet;

/// A function that can be calculated and differentiated, such as SSE or a neural network.
class GDifferentiableFunction
{
public:
	virtual ~GDifferentiableFunction() {}
	
	/// Calulate the output y given the input x.
	virtual void calculateOutput(const GVec &x, GVec &y) = 0;
	
	/// Calculate the derivative of the loss function with respect to this function's parameters given the input x and error err.
	/// This method assumes that calculateOutput has already been called to determine err.
	virtual void updateGradient(const GVec &x, const GVec &err, GVec &gradient) = 0;
};

/// A parametrized differentiable function that can be optimized by tuning the parameters.
class GOptimizableFunction : public GDifferentiableFunction
{
public:
	virtual ~GOptimizableFunction() {}
	
	/// Apply the deltas to the parameters of the function.
	virtual void applyDeltas(const GVec &deltas) = 0;
	
	/// Count the parameters of this function.
	virtual size_t countParameters() const = 0;
};

/// SSE, the default loss function.
class GSumSquaredErrorFunction : public GDifferentiableFunction
{
public:
	virtual void calculateOutput(const GVec &x, GVec &y) override;
	virtual void updateGradient(const GVec &x, const GVec &err, GVec &gradient) override;
};

/// Wrapper function for a GNeuralNet.
class GNeuralNetFunction : public GOptimizableFunction
{
public:
	GNeuralNetFunction(GNeuralNet &nn) : m_nn(nn) {}
	virtual void calculateOutput(const GVec &x, GVec &y) override;
	virtual void updateGradient(const GVec &x, const GVec &err, GVec &dy) override;
	virtual void applyDeltas(const GVec &deltas) override;
	virtual size_t countParameters() const override;
private:
	GNeuralNet &m_nn;
};

/// Optimizes the parameters of a function given a loss metric.
class GFunctionOptimizer
{
public:
	GFunctionOptimizer(GOptimizableFunction *function = NULL, GDifferentiableFunction *error = NULL);
	virtual ~GFunctionOptimizer();
	
	virtual void setFunction(GOptimizableFunction *function);
	virtual void setError(GDifferentiableFunction *error);
	
	virtual void beginOptimizing(size_t featSize, size_t labSize) = 0;
	virtual void updateGradient(const GVec &feat, const GVec &lab) = 0;
	virtual void scaleGradient(double scale) = 0;
	virtual void applyGradient() = 0;
	
	/// Update and apply the gradient for a single training sample (on-line).
	virtual void optimizeIncremental(const GVec &feat, const GVec &lab);
	
	/// Update and apply the gradient for a single batch in order.
	virtual void optimizeBatch(const GMatrix &features, const GMatrix &labels, size_t start, size_t batchSize = 1);
	
	/// Update and apply the gradient for a single batch in randomized order.
	virtual void optimizeBatch(const GMatrix &features, const GMatrix &labels, GRandomIndexIterator &ii, size_t batchSize = 1);
	
	/// Perform a full training process given the training features and labels.
	virtual void optimize(const GMatrix &features, const GMatrix &labels, size_t epochs = 100, size_t batchesPerEpoch = 10, size_t batchSize = 25, GRand *rand = NULL);
protected:
	GOptimizableFunction *m_function;
	GDifferentiableFunction *m_error;
};

class GSGDOptimizer : public GFunctionOptimizer
{
public:
	GSGDOptimizer(GOptimizableFunction *function = NULL, GDifferentiableFunction *error = NULL);
	virtual void beginOptimizing(size_t featSize, size_t labSize) override;
	virtual void updateGradient(const GVec &feat, const GVec &lab) override;
	virtual void scaleGradient(double scale) override;
	virtual void applyGradient() override;
	
	void setLearningRate(double l)	{ m_learningRate = l; }
	double learningRate() const		{ return m_learningRate; }
	
	void setMomentum(double m)		{ m_momentum = m; }
	double momentum() const			{ return m_momentum; }
private:
	GVec m_pred, m_blame, m_gradient, m_deltas;
	double m_learningRate, m_momentum;
	bool m_ready;
};

class GRMSPropOptimizer : public GFunctionOptimizer
{
public:
	GRMSPropOptimizer(GOptimizableFunction *function = NULL, GDifferentiableFunction *error = NULL);
	virtual void beginOptimizing(size_t featSize, size_t labSize) override;
	virtual void updateGradient(const GVec &feat, const GVec &lab) override;
	virtual void scaleGradient(double scale) override;
	virtual void applyGradient() override;
	
	void setLearningRate(double l)	{ m_learningRate = l; }
	double learningRate() const		{ return m_learningRate; }
	
	void setMomentum(double m)		{ m_momentum = m; }
	double momentum() const			{ return m_momentum; }
	
	void setGamma(double g)			{ m_gamma = g; }
	double gamma() const			{ return m_gamma; }
private:
	GVec m_pred, m_blame, m_gradient, m_deltas, m_meanSquare;
	double m_learningRate, m_momentum, m_gamma, m_epsilon;
	bool m_ready;
};














// MARK: Old GOptimizer.h below

/// The optimizer seeks to find values that minimize this target function.
class GTargetFunction
{
protected:
	GRelation* m_pRelation;

public:
	/// Takes ownership of pRelation
	GTargetFunction(GRelation* pRelation) : m_pRelation(pRelation) {}

	GTargetFunction(size_t dims);

	virtual ~GTargetFunction();

	/// Returns a (smart) pointer to the relation, which specifies the type
	/// (discrete or real) of each element in the vector that is being optimized.
	GRelation* relation() { return m_pRelation; }

	/// This method should return true if computeError is deterministic with respect to
	/// the vector being optimized. It should return false if the error depends on some
	/// state other than the vector being optimized. This mostly affects whether
	/// the optimization algorithms are permitted to remember old error values for
	/// efficiency purposes. Stable is assumed, so you should override this method if
	/// your target function is not stable.
	virtual bool isStable() { return true; }

	/// Sets pVector to an initial guess. The default behavior is to initialize the
	/// vector to all zeros. You should override this method if different behavior
	/// is desired.
	virtual void initVector(GVec& vector);

	/// Computes the error of the given vector using all patterns
	virtual double computeError(const GVec& vector) = 0;
};


#ifndef MIN_PREDICT
class GOptimizerBasicTestTargetFunction : public GTargetFunction
{
public:
	GOptimizerBasicTestTargetFunction() : GTargetFunction(3) {}

	virtual double computeError(const GVec& vector);
};
#endif // MIN_PREDICT


/// This is the base class of all search algorithms
/// that can jump to any vector in the search space
/// seek the vector that minimizes error.
class GOptimizer
{
protected:
	GTargetFunction* m_pCritic;

public:
	GOptimizer(GTargetFunction* pCritic);
	virtual ~GOptimizer();

	/// Makes another attempt to find a better vector. Returns
	/// the heuristic error. (Usually you will call this method
	/// in a loop until your stopping criteria has been met.)
	virtual double iterate() = 0;

	/// Returns the current vector of the optimizer. For greedy search
	/// methods, this will be the best vector yet found.
	virtual const GVec& currentVector() = 0;

	/// This will first call iterate() nBurnInIterations times,
	/// then it will repeatedly call iterate() in blocks of
	/// nIterations times. If the error heuristic has not improved
	/// by the specified ratio after a block of iterations, it will
	/// stop. (For example, if the error before the block of iterations
	/// was 50, and the error after is 49, then training will stop
	/// if dImprovement is > 0.02.) If the error heuristic is not
	/// stable, then the value of nIterations should be large.
	double searchUntil(size_t nBurnInIterations, size_t nIterations, double dImprovement);

#ifndef MIN_PREDICT
	/// This is a helper method used by the unit tests of several model learners
	void basicTest(double minAccuracy, double warnRange = 0.001);
#endif // MIN_PREDICT

};



/// This class simplifies simultaneously solving several optimization problems
class GParallelOptimizers
{
protected:
	GRelation* m_pRelation;
	std::vector<GTargetFunction*> m_targetFunctions;
	std::vector<GOptimizer*> m_optimizers;

public:
	/// If the problems all have the same number of dims, and they're all continuous, you can call
	/// relation() to get a relation for constructing the target functions. Otherwise, use dims=0
	/// and don't call relation().
	GParallelOptimizers(size_t dims);
	~GParallelOptimizers();

	/// Returns the relation associated with these optimizers
	GRelation* relation() { return m_pRelation; }

	/// Takes ownership of pTargetFunction and pOptimizer
	void add(GTargetFunction* pTargetFunction, GOptimizer* pOptimizer);

	/// Returns a vector of pointers to the optimizers
	std::vector<GOptimizer*>& optimizers() { return m_optimizers; }

	/// Returns a vector of pointers to the target functions
	std::vector<GTargetFunction*>& targetFunctions() { return m_targetFunctions; }

	/// Perform one iteration on all of the optimizers
	double iterateAll();

	/// Optimize until the specified conditions are met
	double searchUntil(size_t nBurnInIterations, size_t nIterations, double dImprovement);
};



class GActionPathState
{
friend class GActionPath;
public:
	GActionPathState() {}
	virtual ~GActionPathState() {}

protected:
	/// Performs the specified action on the state. (so pState holds
	/// both input and output data.) This method is protected because
	/// you should call GActionPath::doAction, and it will call this method.
	virtual void performAction(size_t nAction) = 0;

	/// Creates a deep copy of this state object
	virtual GActionPathState* copy() = 0;

protected:
	/// Evaluate the error of the given path. Many search algorithms
	/// (like GAStarSearch) rely heavily on the heuristic to make the search effective.
	/// For example, if you don't penalize redundant paths to the same state, the search
	/// space becomes exponential and therefore impossible to search. So a good critic
	/// must keep track of which states have already been visited, severely penalize longer
	/// paths to a state that has already been visited by a shorter path, and will carefully
	/// balance between path length and distance from the goal in producing the error value.
	virtual double critiquePath(size_t nPathLen, GAction* pLastAction) = 0;
};




class GActionPath
{
protected:
	GActionPathState* m_pHeadState;
	GAction* m_pLastAction;
	size_t m_nPathLen;

public:
	/// Takes ownership of pState
	GActionPath(GActionPathState* pState);
	~GActionPath();

	/// Makes a copy of this path
	GActionPath* fork();

	/// Returns the number of actions in the path
	size_t length() { return m_nPathLen; }

	/// Gets the first nCount actions of the specified path
	void path(size_t nCount, size_t* pOutBuf);

	/// Returns the head-state of the path
	GActionPathState* state() { return m_pHeadState; }

	/// Adds the specified action to the path and modifies the head state accordingly
	void doAction(size_t nAction);

	/// Computes the error of this path
	double critique();
};




/// This is the base class of search algorithms that can
/// only perform a discreet set of actions (as opposed to jumping
/// to anywhere in the search space), and seeks to minimize the
/// error of a path of actions
class GActionPathSearch
{
protected:
	size_t m_nActionCount;

public:
	/// Takes ownership of pStartState
	GActionPathSearch(GActionPathState* pStartState, size_t nActionCount)
	{
		GAssert(nActionCount > 1); // not enough actions for a meaningful search")
		m_nActionCount = nActionCount;
	}

	virtual ~GActionPathSearch()
	{
	}

	/// Returns the number of possible actions
	inline size_t actionCount() { return m_nActionCount; }

	/// Call this in a loop to do the searching. If it returns
	/// true, then it's done so don't call it anymore.
	virtual bool iterate() = 0;

	/// Returns the best known path so far
	virtual GActionPath* bestPath() = 0;

	/// Returns the error of the best known path
	virtual double bestPathError() = 0;
};


} // namespace GClasses

#endif // __GSEARCH_H__
