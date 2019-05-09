class ImageSubstractor
{
public:
	ImageSubstractor();
	~ImageSubstractor();

	void Loop();
	const bool GetRunState() const { return m_IsRunning; };

private:
	cv::VideoCapture m_pWebCam = nullptr;
	bool m_IsRunning = false;

	const int TIMEOUT = 30;
	int m_Threshold = 30;

	const bool SetWebcam();
	void StoreFrame();

	const cv::Mat CalculateDifference();
	const cv::Mat CalculateThreshold(cv::Mat& diffImage);

	std::deque<cv::Mat> m_VecImages;

	cv::Mat m_BgImage = cv::Mat();

	bool m_IsBgSet = false;

	void SetBgImage();
	const bool CheckMotionThreshold(cv::Mat& threshImg);

	int m_MaxMotionValue = 40;

	int m_MinROIMotionValue  = 30;
	int m_MinPrevROIMotionValue = 20;

	cv::Rect DrawRegionOfIntrest(double xPos, double yPos, double width, double height, cv::Mat& img);
	const bool DetectMotionInROI(cv::Rect roi, cv::Mat threshImage);

	cv::Rect m_RegionOfIntrest1;
	cv::Rect m_RegionOfIntrest2;

	cv::Mat m_RoiChangeImage;

	cv::Mat m_AnimationImage;

	cv::Mat GetPartOfImage(bool isRunning);
	int m_RunFrame = 0;
	int m_JumpFrame = 0;
	cv::Mat m_PartAniToShow;
};