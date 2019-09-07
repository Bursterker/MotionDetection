class ImageSubstractor
{
#pragma region Life Cycle
public:
	ImageSubstractor();
	~ImageSubstractor();

	void Initialize();
	void Loop();
#pragma endregion

#pragma region Settings
public:
	const bool GetRunState() const { return m_IsRunning; };
	bool m_IsInitialized = false;

private:
	const bool SetWebcam();

	cv::VideoCapture m_pWebCam = nullptr;
	bool m_IsRunning = false;

	std::string window_TestOrg = "Original Test";
	std::string window_TestDiff = "Diff Test";
	std::string window_TestThresh = "Threshold Test";
#pragma endregion

#pragma region Capture Settings
private:
	static const int TIMEOUT = 30;
	int m_Threshold = 30;

	int m_MaxMotionValue = 40;

	int m_MinROIMotionValue  = 30;
	int m_MinPrevROIMotionValue = 20;

	cv::Rect m_RegionOfIntrest1;
	cv::Rect m_RegionOfIntrest2;
#pragma endregion

#pragma region Image Processing
private:
	void StoreFrame();
	void DisplayBlurredImage();
	cv::Mat DisplayDifferenceImage();
	cv::Mat DisplayThresholdImage(cv::Mat image);

	void DisplayMotionImage(cv::Mat image);
	void DetectMotion(cv::Mat image);
	void DisplayAnimation(bool motionInROI1, bool motionInROI2);

	const cv::Mat CalculateDifference();
	const cv::Mat CalculateThreshold(cv::Mat& diffImage);

	std::deque<cv::Mat> m_StoredFrames;

	cv::Mat m_BgImage = cv::Mat();

	bool m_IsBgSet = false;

	void SetBgImage();
	const bool CheckMotionThreshold(cv::Mat& threshImg);

	cv::Rect DrawRegionOfIntrest(double xPos, double yPos, double width, double height, cv::Mat& img);
	const bool DetectMotionInROI(cv::Rect roi, cv::Mat threshImage);

	cv::Mat m_RoiChangeImage;
#pragma endregion

#pragma region Animation
private:
	cv::Mat m_AnimationImage;

	cv::Mat GetPartOfImage(bool isRunning);
	int m_RunFrame = 0;
	int m_JumpFrame = 0;
	cv::Mat m_PartAniToShow;
#pragma endregion
};