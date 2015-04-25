#include "CVector.h"

#define FUNC_CVector__Magnitude 0x4082C0
#define FUNC_CVector__Normalise 0x59C910
#define FUNC_CVector__NormaliseGetLength 0x59C970
#define FUNC_CVector__Cross 0x70F890
#define FUNC_CVector__Sum 0x40FDD0
#define FUNC_CVector__Difference 0x40FE00
#define FUNC_CVector__operatorAssignmentCVector 0x411B50
#define FUNC_CVector__operatorAdditionAssignmentCVector 0x411A00
#define FUNC_CVector__operatorMultiplicationAssignmentFloat 0x40FEF0
#define FUNC_CVector__operatorDivisionAssignmentFloat 0x0411A30

CVector::CVector()
{

}

CVector::CVector(float x, float y, float z)
{
	((void(__thiscall *)(CVector *, float, float, float))0x420B10)(this, x, y, z);
}

// Returns length of vector
float CVector::Magnitude()
{
	return ((float (__thiscall*)(CVector*)) FUNC_CVector__Magnitude)(this);
}

float CVector::Magnitude2D()
{
	return ((float (__thiscall *)(CVector *))0x406D50)(this);
}

// Normalises a vector
void CVector::Normalise()
{
	((void (__thiscall*)(CVector*)) FUNC_CVector__Normalise)(this);
}

// Normalises a vector and returns length
float CVector::NormaliseAndMag()
{
	return ( (float (__thiscall*)(CVector*)) FUNC_CVector__NormaliseGetLength)(this);
}

// Performs cross calculation
void CVector::Cross(const CVector& left, const CVector &right)
{
	((void (__thiscall*)(CVector*, const CVector& left, const CVector &right)) FUNC_CVector__Cross)(this, left, right);
}

// Adds left + right and stores result
void CVector::Sum(const CVector& left, const CVector &right)
{
	( (void (__thiscall*)(CVector*, const CVector& left, const CVector &right)) FUNC_CVector__Sum)(this, left, right);
}

// Subtracts left - right and stores result
void CVector::Difference(const CVector& left, const CVector &right)
{
	((void (__thiscall*)(CVector*, const CVector& left, const CVector &right)) FUNC_CVector__Difference)(this, left, right);
}

// Assigns value from other vector
void CVector::operator= (const CVector& right)
{
	((void (__thiscall*)(CVector*, const CVector &right)) FUNC_CVector__operatorAssignmentCVector)(this, right);
}

// Adds value from the second vector.
void CVector::operator+=(const CVector& right)
{
	((void (__thiscall*)(CVector*, const CVector &right)) FUNC_CVector__operatorAdditionAssignmentCVector)(this, right);
}

void CVector::operator-=(const CVector& right)
{
	((void (__thiscall *)(CVector *, const CVector& right))0x406D70)(this, right);
}

// Multiplies vector by a floating point value
void CVector::operator *= (float multiplier)
{
	((void (__thiscall*)(CVector*, float multiplier)) FUNC_CVector__operatorMultiplicationAssignmentFloat)(this, multiplier);
}

// Divides vector by a floating point value
void CVector::operator /= (float divisor)
{
	((void (__thiscall*)(CVector*, float divisor)) FUNC_CVector__operatorDivisionAssignmentFloat)(this, divisor);
}

void CVector::FromMultiply(CMatrix  const& matrix, CVector const& vector)
{
	((void (__thiscall *)(CVector *, CMatrix  const&, CVector const&))0x59C670)(this, matrix, vector);
}

void CVector::FromMultiply3x3(CMatrix  const& matrix, CVector const& vector)
{
	((void (__thiscall *)(CVector *, CMatrix  const&, CVector const&))0x59C6D0)(this, matrix, vector);
}