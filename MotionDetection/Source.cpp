#include "stdafx.h"
#include "Projects\ImageSubstractor.h"

int main()
{
	ImageSubstractor* m_pImageSubstractor = new ImageSubstractor();

	while (m_pImageSubstractor->GetRunState())
	{
		m_pImageSubstractor->Loop();
	}

	return 0;
}