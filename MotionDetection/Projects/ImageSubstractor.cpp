#include "../stdafx.h"
#include "ImageSubstractor.h"

ImageSubstractor::ImageSubstractor()
{
	m_IsRunning = SetWebcam();
	m_AnimationImage = cv::imread("MegamanAnimation.jpg");
}

ImageSubstractor::~ImageSubstractor()
{
}

void ImageSubstractor::Loop()
{
	//SetBackGroundImage
	if (m_IsBgSet == false)
	{
		std::string input;
		std::getline(std::cin, input);
		std::transform(input.begin(), input.end(), input.begin(), ::tolower);

		if (input == "sb" | input == "cb")
		{
			SetBgImage();
			m_IsBgSet = true;
		}
		else
		{
			return;
		}
	}

	//Store new Frame
	StoreFrame();

	// Display the frame
	std::string window_TestOrg = "Original Test";
	std::string window_TestDiff = "Diff Test";
	std::string window_TestThresh = "Threshold Test";

	//Create a new window
	cv::namedWindow(window_TestOrg, CV_WINDOW_AUTOSIZE);
	cv::namedWindow(window_TestDiff, CV_WINDOW_AUTOSIZE);
	cv::namedWindow(window_TestThresh, CV_WINDOW_AUTOSIZE);

	//Display the original image on a window
	if (m_VecImages.size() > 0)
	{
		cv::GaussianBlur(m_VecImages[m_VecImages.size() - 1], m_VecImages[m_VecImages.size() - 1], cv::Size(5, 5), 0);

		m_RegionOfIntrest1 = DrawRegionOfIntrest(0.0, 0.0, 0.3, 0.3, m_VecImages[m_VecImages.size() - 1]);
		m_RegionOfIntrest2 = DrawRegionOfIntrest(0.7, 0.0, 0.3, 0.3, m_VecImages[m_VecImages.size() - 1]);

		cv::imshow(window_TestOrg, m_VecImages[m_VecImages.size() - 1]);
	}

	//Display the difference image on a window
	cv::Mat diffImage = CalculateDifference();
	if (diffImage.empty() == false)
	{
		cv::imshow(window_TestDiff, diffImage);
	}

	//Display the thresholded image on a window
	cv::Mat threshImage = CalculateThreshold(diffImage);
	if (threshImage.empty() == false)
	{
		cv::createTrackbar("Thresh value", window_TestThresh, &m_Threshold, 255);
		cv::createTrackbar("Motion Thresh value", window_TestThresh, &m_MaxMotionValue, 100);
		cv::createTrackbar("Min ROI motion value", window_TestThresh, &m_MinROIMotionValue, 100);
		cv::createTrackbar("Min prev ROI motion value", window_TestThresh, &m_MinPrevROIMotionValue, 100);

		cv::imshow(window_TestThresh, threshImage);
	}

	//Check if there was too much movement
	bool motionInRoi1 = false, motionInRoi2 = false;

	if (CheckMotionThreshold(threshImage) == false)
	{
		//Check if there is motion in ROI
		motionInRoi1 = DetectMotionInROI(m_RegionOfIntrest1, threshImage);
		motionInRoi2 = DetectMotionInROI(m_RegionOfIntrest2, threshImage);
	}

	//Display the ChangedMovement image on a window
	if (m_RoiChangeImage.empty() == false)
	{
		cv::imshow("ROI change", m_RoiChangeImage);
	}

	//Get a part of the enimation sheet
	if (motionInRoi1)
	{
		m_PartAniToShow = GetPartOfImage(true);

		m_RunFrame++;

		std::cout << "Motion in region 1" << std::endl;
	}
	if (motionInRoi2)
	{
		m_PartAniToShow = GetPartOfImage(false);

		m_JumpFrame++;

		std::cout << "Motion in region 2" << std::endl;
	}

	//Display the Animated image on a window
	if (m_PartAniToShow.empty() == false)
	{
		cv::imshow("animation", m_PartAniToShow);
	}

	//Give openCV some time to process everyting
	cv::waitKey(TIMEOUT);
}


const bool ImageSubstractor::SetWebcam()
{
	//CREATE a new webcam
	m_pWebCam = cv::VideoCapture(0);

	//CHECK if webcam works
	if (m_pWebCam.isOpened() == false)
	{
		std::cout << "ERROR: Could not open the webcam!" << std::endl;
		return false;
	}

	std::cout << "Webcam succesfully set!" << std::endl;
	return true;
}

void ImageSubstractor::StoreFrame()
{
	cv::Mat currFrame = cv::Mat();

	bool isFrameRead = m_pWebCam.read(currFrame);

	if (isFrameRead)
	{
		m_VecImages.push_back(currFrame);

		if (m_VecImages.size() >= 3)
		{
			m_VecImages[0].release();
			m_VecImages.pop_front();
		}
		return;
	}
	else
	{
		std::cout << "WARNING: Could not read frame!" << std::endl;
		std::cout << "WARNING: Frame not stored!" << std::endl;
	}
}

const cv::Mat ImageSubstractor::CalculateDifference()
{
	if (m_VecImages.size() < 2) return cv::Mat();

	cv::Mat firstImage = m_VecImages[m_VecImages.size() - 2].clone();
	cv::Mat secondImage = m_VecImages[m_VecImages.size() - 1].clone();

	firstImage = m_BgImage.clone();

	cv::Mat diffImage = cv::Mat();

	//ConvertToGrayScale
	cv::cvtColor(firstImage, firstImage, CV_BGR2GRAY);
	cv::cvtColor(secondImage, secondImage, CV_BGR2GRAY);

	//AplyABlurr for better result (less noise)
	int blurSize = 5;

	cv::medianBlur(firstImage, firstImage, blurSize);
	cv::medianBlur(secondImage, secondImage, blurSize);

	//CalculateDiffrences
	cv::absdiff(firstImage, secondImage, diffImage);

	return diffImage;
}

const cv::Mat ImageSubstractor::CalculateThreshold(cv::Mat& diffImage)
{
	cv::Mat threshImage = cv::Mat();

	//Determine a threshold
	cv::threshold(diffImage, threshImage, m_Threshold, 255.0, CV_THRESH_BINARY);

	return threshImage;
}

void ImageSubstractor::SetBgImage()
{
	cv::Mat currFrame = cv::Mat();

	m_pWebCam.read(currFrame);
	bool isFrameRead = m_pWebCam.retrieve(currFrame);

	if (isFrameRead)
	{
		m_BgImage = currFrame;
	}
	else
	{
		std::cout << "WARNING: Could not read background frame!" << std::endl;
	}
}

const bool ImageSubstractor::CheckMotionThreshold(cv::Mat& threshImg)
{
	int totPixels = threshImg.rows * threshImg.cols;
	int aboveThresh = 1; //setTo1 for no division by 0

	typedef uchar Pixel;

	for (int rows = 0; rows < threshImg.rows; rows++)
	{
		for (int cols = 0; cols < threshImg.cols; cols++)
		{
			if (threshImg.at<uchar>(rows, cols) > 0)
			{
				aboveThresh++;
			}
		}
	}

	double val1 = totPixels / aboveThresh;
	double percentAboveThresh = 100 / val1;

	if (percentAboveThresh > m_MaxMotionValue)
	{
		SetBgImage();
		return true;
	}
	return false;
}
cv::Rect ImageSubstractor::DrawRegionOfIntrest(double xPos, double yPos, double width, double height, cv::Mat& img)
{
	double imgWidth = img.size().width;
	double imgHeight = img.size().height;

	cv::Rect roi = cv::Rect(cv::Point(imgWidth * xPos, imgHeight * yPos), cv::Point(imgWidth * (xPos + width), imgHeight * (yPos + height)));

	cv::rectangle(img, roi, cv::Scalar(40, 80, 200), 3);

	return roi;
}
const bool ImageSubstractor::DetectMotionInROI(cv::Rect roi, cv::Mat threshImage)
{
	int pixelsWhite = cv::countNonZero(threshImage(roi));

	pixelsWhite++; // Ignore integer division by 0

	int val1 = (roi.width * roi.height) / pixelsWhite;

	int pixelPercentage = 100 / val1;

	if (pixelPercentage > m_MinROIMotionValue)
	{
		//Compare if there is atleast a xx% pixel difference with last frame
		cv::Mat currFrame = m_VecImages[m_VecImages.size() - 1];
		cv::Mat prevFrame = m_VecImages[m_VecImages.size() - 2];

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
		{
			m_RunFrame = 0;
		}

		point1.x = m_RunFrame * width;
		point2.x = m_RunFrame * width + width;

		point1.y = 0;
		point2.y = height;
	}
	else
	{
		if (m_JumpFrame >= totalFrames)
		{
			m_JumpFrame = 0;
		}

		point1.x = m_JumpFrame * width;
		point2.x = m_JumpFrame * width + width;

		point1.y = height;
		point2.y = height * 2;
	}

	return m_AnimationImage(cv::Rect(point1, point2));
}
