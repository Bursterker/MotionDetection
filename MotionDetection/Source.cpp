#include "stdafx.h"
#include "Projects\ImageSubstractor.h"

int main()
{
	ImageSubstractor* m_pImageSubstractor = new ImageSubstractor();

	// Check if the ImageSubstractor is running.
	if(m_pImageSubstractor->GetRunState() == false)
		return 1;

	// Halt the progress of the program until, the user initializes the image substractor.
	while (m_pImageSubstractor->m_IsInitialized == false)
	{
			m_pImageSubstractor->Initialize();
	}

	// Execute the main loop of the image substractor for as long it is running.
	while (m_pImageSubstractor->GetRunState())
	{
		m_pImageSubstractor->Loop();
	}

	return 0;
}