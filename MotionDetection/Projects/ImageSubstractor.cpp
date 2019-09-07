#include "../stdafx.h"
#include "ImageSubstractor.h"

#pragma region Life Cycle
ImageSubstractor::ImageSubstractor()
{
	m_IsRunning = SetWebcam();

	m_AnimationImage = cv::imread("MegamanAnimation.jpg");
}

//=============================================================//
ImageSubstractor::~ImageSubstractor()
{
}

//=============================================================//
void ImageSubstractor::Initialize()
{
	// Capture the background image, when the user tells the system to capture the background.
	if (m_IsBgSet == false)
	{
		// Capture the user input.
		std::string input;
		std::getline(std::cin, input);
		std::transform(input.begin(), input.end(), input.begin(), ::tolower);

		// Set-up the background.
		if (input == "sb" | input == "cb")
		{
			SetBgImage();
			m_IsBgSet = true;
		}
		else
			return;
	}

	// Create a new window
	cv::namedWindow(window_TestOrg, CV_WINDOW_AUTOSIZE);
	cv::namedWindow(window_TestDiff, CV_WINDOW_AUTOSIZE);
	cv::namedWindow(window_TestThresh, CV_WINDOW_AUTOSIZE);

	m_IsInitialized = true;
}

//=============================================================//
void ImageSubstractor::Loop()
{
	// Store a new frame.
	StoreFrame();

	// Display the blurred image on a window.
	DisplayBlurredImage();

	// Generate and display the difference image on a window.
	cv::Mat diffImage = DisplayDifferenceImage();

	// Generate and display the thresholded image on a window.
	cv::Mat threshImage = DisplayThresholdImage(diffImage);

	// Generate and display the motion image on a window.
	DisplayMotionImage(threshImage);

	// Give openCV some time to process everyting.
	cv::waitKey(TIMEOUT);
}
#pragma endregion

#pragma region Image Processing
void ImageSubstractor::DisplayBlurredImage()
{
	if (m_StoredFrames.size() > 0)
	{
		// Apply a gaussian blur on the image.
		cv::GaussianBlur(m_StoredFrames[m_StoredFrames.size() - 1], m_StoredFrames[m_StoredFrames.size() - 1], cv::Size(5, 5), 0);

		// Draw ROI squares over the image.
		m_RegionOfIntrest1 = DrawRegionOfIntrest(0.0, 0.0, 0.3, 0.3, m_StoredFrames[m_StoredFrames.size() - 1]);
		m_RegionOfIntrest2 = DrawRegionOfIntrest(0.7, 0.0, 0.3, 0.3, m_StoredFrames[m_StoredFrames.size() - 1]);

		cv::imshow(window_TestOrg, m_StoredFrames[m_StoredFrames.size() - 1]);
	}
}

//=============================================================//
cv::Mat ImageSubstractor::DisplayDifferenceImage()
{
	cv::Mat diffImage = CalculateDifference();

	if (diffImage.empty() == false)
		cv::imshow(window_TestDiff, diffImage);

	return diffImage;
}

//=============================================================//
cv::Mat ImageSubstractor::DisplayThresholdImage(cv::Mat image)
{
	cv::Mat threshImage = CalculateThreshold(image);

	if (threshImage.empty() == false)
	{
		// Create sliders to change the threshold values.
		cv::createTrackbar("Thresh value", window_TestThresh, &m_Threshold, 255);
		cv::createTrackbar("Motion Thresh value", window_TestThresh, &m_MaxMotionValue, 100);
		cv::createTrackbar("Min ROI motion value", window_TestThresh, &m_MinROIMotionValue, 100);
		cv::createTrackbar("Min prev ROI motion value", window_TestThresh, &m_MinPrevROIMotionValue, 100);

		cv::imshow(window_TestThresh, threshImage);
	}

	return threshImage;
}

//=============================================================//
void ImageSubstractor::DisplayMotionImage(cv::Mat image)
{
	DetectMotion(image);

	if (m_RoiChangeImage.empty() == false)
	{
		cv::imshow("ROI change", m_RoiChangeImage);
	}
}

//=============================================================//
void ImageSubstractor::DetectMotion(cv::Mat image)
{
	bool motionInRoi1 = false, motionInRoi2 = false;

	// Check if there is not an overload of motion in the image.
	if (CheckMotionThreshold(image) == false)
	{
		// Check if there is motion in ROI.
		motionInRoi1 = DetectMotionInROI(m_RegionOfIntrest1, image);
		motionInRoi2 = DetectMotionInROI(m_RegionOfIntrest2, image);
	}

	DisplayAnimation(motionInRoi1, motionInRoi2);
}

//=============================================================//
void ImageSubstractor::DisplayAnimation(bool motionInROI1, bool motionInROI2)
{
	// Get a part of the animation sheet to show depending on where motion is detected.
	if (motionInROI1)
	{
		m_PartAniToShow = GetPartOfImage(true);

		m_RunFrame++;

		std::cout << "Motion in region 1" << std::endl;
	}
	else if (motionInROI2)
	{
		m_PartAniToShow = GetPartOfImage(false);

		m_JumpFrame++;

		std::cout << "Motion in region 2" << std::endl;
	}

	if (m_PartAniToShow.empty() == false)
	{
		cv::imshow("animation", m_PartAniToShow);
	}
}

//=============================================================//
const cv::Mat ImageSubstractor::CalculateDifference()
{
	// If there are not enough stored frames, no difference can be calculated.
	if (m_StoredFrames.size() < 2) 
		return cv::Mat();

	cv::Mat firstImage = m_StoredFrames[m_StoredFrames.size() - 2].clone();
	cv::Mat secondImage = m_StoredFrames[m_StoredFrames.size() - 1].clone();

	firstImage = m_BgImage.clone();

	cv::Mat diffImage = cv::Mat();

	// Convert the images to gray scale.
	cv::cvtColor(firstImage, firstImage, CV_BGR2GRAY);
	cv::cvtColor(secondImage, secondImage, CV_BGR2GRAY);

	// Apply a median blur to reduce noise.
	int blurSize = 5;

	cv::medianBlur(firstImage, firstImage, blurSize);
	cv::medianBlur(secondImage, secondImage, blurSize);

	// Calculate the difference in between the images.
	cv::absdiff(firstImage, secondImage, diffImage);

	return diffImage;
}
//=============================================================//
const cv::Mat ImageSubstractor::CalculateThreshold(cv::Mat& diffImage)
{
	cv::Mat threshImage = cv::Mat();

	// Generate a threshold image. (results in a matrix of values 0 or 1)
	cv::threshold(diffImage, threshImage, m_Threshold, 255.0, CV_THRESH_BINARY);

	return threshImage;
}
#pragma endregion

#pragma region Methods
const bool ImageSubstractor::SetWebcam()
{
	// Create a new webcam input stream.
	m_pWebCam = cv::VideoCapture(0);

	// Check if video can be captured from the webcam.
	if (m_pWebCam.isOpened() == false)
	{
		std::cout << "ERROR: Could not open the webcam!" << std::endl;
		return false;
	}

	std::cout << "Webcam succesfully set!" << std::endl;
	return true;
}

//=============================================================//
void ImageSubstractor::StoreFrame()
{
	cv::Mat currFrame = cv::Mat();

	bool isFrameRead = m_pWebCam.read(currFrame);

	if (isFrameRead)
	{
		m_StoredFrames.push_back(currFrame);

		// Release frames when not needed anymore.
		if (m_StoredFrames.size() >= 3)
		{
			m_StoredFrames[0].release();
			m_StoredFrames.pop_front();
		}

		return;
	}

	std::cout << "WARNING: Could not read frame!" << std::endl;
	std::cout << "WARNING: Frame not stored!" << std::endl;
}

//=============================================================//
void ImageSubstractor::SetBgImage()
{
	cv::Mat currFrame = cv::Mat();

	m_pWebCam.read(currFrame);
	bool isFrameRead = m_pWebCam.retrieve(currFrame);

	if (isFrameRead)
		m_BgImage = currFrame;
	else
		std::cout << "WARNING: Could not read background frame!" << std::endl;
}

//=============================================================//
const bool ImageSubstractor::CheckMotionThreshold(cv::Mat& threshImg)
{
	int totPixels = threshImg.rows * threshImg.cols;
	int pixelsAboveThresh = 1;

	// Retrieve the amount of pixels above the threshold.
	for (int rows = 0; rows < threshImg.rows; rows++)
	{
		for (int cols = 0; cols < threshImg.cols; cols++)
		{
			if (threshImg.at<uchar>(rows, cols) > 0)
				pixelsAboveThresh++;
		}
	}

	double val1 = totPixels / pixelsAboveThresh;
	double percentAboveThresh = 100 / val1;

	// Generate a new background if there is too much motion detected.
	if (percentAboveThresh > m_MaxMotionValue)
	{
		SetBgImage();

		return true;
	}

	return false;
}

//=============================================================//
cv::Rect ImageSubstractor::DrawRegionOfIntrest(double xPos, double yPos, double width, double height, cv::Mat& img)
{
	double imgWidth = img.size().width;
	double imgHeight = img.size().height;

	cv::Rect roi = cv::Rect(cv::Point(imgWidth * xPos, imgHeight * yPos), cv::Point(imgWidth * (xPos + width), imgHeight * (yPos + height)));

	cv::rectangle(img, roi, cv::Scalar(40, 80, 200), 3);

	return roi;
}

//=============================================================//
const bool ImageSubstractor::DetectMotionInROI(cv::Rect roi, cv::Mat threshImage)
{
	int pixelsWhite = cv::countNonZero(threshImage(roi));

	// Increase the white pixel count by 1 to ignore division by 0.
	pixelsWhite++;

	int val1 = (roi.width * roi.height) / pixelsWhite;

	int pixelPercentage = 100 / val1;

	if (pixelPercentage > m_MinROIMotionValue)
	{
		// Compare if there is atleast a xx% pixel difference with last frame.
		cv::Mat currFrame = m_StoredFrames[m_StoredFrames.size() - 1];
		cv::Mat prevFrame = m_StoredFrames[m_StoredFrames.size() - 2];

		cv::cvtColor(currFrame, currFrame, CV_BGR2GRAY);
		cv::cvtColor(prevFrame, prevFrame, CV_BGR2GRAY);

		cv::Mat absImage;
		cv::absdiff(currFrame, prevFrame, absImage);

		cv::threshold(absImage, m_RoiChangeImage, m_Threshold, 255.0, CV_THRESH_BINARY);

		int prevChangePixels = cv::countNonZero(m_RoiChangeImage(roi));
		prevChangePixels++;

		val1 = (roi.width * roi.height) / prevChangePixels;
		pixelPercentage = 100 / val1;

		if (pixelPercentage > m_MinPrevROIMotionValue)
		{
			return true;
		}
	}

	return false;
}

//=============================================================//
cv::Mat ImageSubstractor::GetPartOfImage(bool isRunning)
{
	int totalFrames = 9;

	int width = m_AnimationImage.size().width / totalFrames;
	int height = m_AnimationImage.size().height / 2;

	cv::Point point1;
	cv::Point point2;

	if (isRunning)
	{
		if (m_RunFrame >= totalFrames)
			m_RunFrame = 0;

		point1.x = m_RunFrame * width;
		point2.x = m_RunFrame * width + width;

		point1.y = 0;
		point2.y = height;
	}
	else
	{
		if (m_JumpFrame >= totalFrames)
			m_JumpFrame = 0;

		point1.x = m_JumpFrame * width;
		point2.x = m_JumpFrame * width + width;

		point1.y = height;
		point2.y = height * 2;
	}

	return m_AnimationImage(cv::Rect(point1, point2));
}
#pragma endregion
