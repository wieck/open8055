#pragma once


namespace Open8055Demo {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Runtime::InteropServices;

	/// <summary>
	/// Summary for Form1
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{
	public:
		Form1(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}

			Form1_Destroy();
		}
	private: System::Windows::Forms::TextBox^  cardDestination;
	protected: 

	protected: 
	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::Button^  buttonConnect;

	private: System::Windows::Forms::Label^  connectedMessage;
	private: System::Windows::Forms::TextBox^  cardPassword;
	private: System::Windows::Forms::Label^  label2;

	private: System::Windows::Forms::Label^  label3;
	private: System::Windows::Forms::TextBox^  ADC1;
	private: System::Windows::Forms::TextBox^  ADC2;
	private: System::Windows::Forms::Label^  label4;
	private: System::Windows::Forms::CheckBox^  I1;
	private: System::Windows::Forms::Label^  label5;
	private: System::Windows::Forms::Label^  label6;
	private: System::Windows::Forms::TextBox^  Counter1;

	private: System::Windows::Forms::Label^  label7;
	private: System::Windows::Forms::TextBox^  Debounce1;
	private: System::Windows::Forms::Button^  SetDebounce1;
	private: System::Windows::Forms::Label^  label8;
	private: System::Windows::Forms::CheckBox^  I2;

	private: System::Windows::Forms::TextBox^  Counter2;
	private: System::Windows::Forms::Label^  label9;
	private: System::Windows::Forms::Label^  label10;
	private: System::Windows::Forms::TextBox^  Debounce2;
	private: System::Windows::Forms::Button^  SetDebounce2;
	private: System::Windows::Forms::Button^  SetDebounce3;

	private: System::Windows::Forms::TextBox^  Debounce3;

	private: System::Windows::Forms::Label^  label11;
	private: System::Windows::Forms::TextBox^  Counter3;

	private: System::Windows::Forms::Label^  label12;
	private: System::Windows::Forms::Label^  label13;
	private: System::Windows::Forms::CheckBox^  I3;
	private: System::Windows::Forms::Button^  SetDebounce4;

	private: System::Windows::Forms::TextBox^  Debounce4;

	private: System::Windows::Forms::Label^  label14;
	private: System::Windows::Forms::TextBox^  Counter4;
	private: System::Windows::Forms::Label^  label15;
	private: System::Windows::Forms::Label^  label16;
	private: System::Windows::Forms::CheckBox^  I4;
	private: System::Windows::Forms::Button^  SetDebounce5;
	private: System::Windows::Forms::TextBox^  Debounce5;
	private: System::Windows::Forms::Label^  label17;
	private: System::Windows::Forms::TextBox^  Counter5;
	private: System::Windows::Forms::Label^  label18;
	private: System::Windows::Forms::Label^  label19;
	private: System::Windows::Forms::CheckBox^  I5;
	private: System::Windows::Forms::Button^  CounterReset1;
	private: System::Windows::Forms::Button^  CounterReset5;
	private: System::Windows::Forms::Button^  CounterReset4;
	private: System::Windows::Forms::Button^  CounterReset3;
	private: System::Windows::Forms::Button^  CounterReset2;
	private: System::Windows::Forms::ComboBox^  InputMode1;
	private: System::Windows::Forms::ComboBox^  InputMode2;
	private: System::Windows::Forms::ComboBox^  InputMode3;
	private: System::Windows::Forms::ComboBox^  InputMode4;
	private: System::Windows::Forms::ComboBox^  InputMode5;
	private: System::Windows::Forms::ProgressBar^  ADCBar1;
	private: System::Windows::Forms::ProgressBar^  ADCBar2;
	private: System::Windows::Forms::Label^  label20;
	private: System::Windows::Forms::TextBox^  PWM1;


	private: System::Windows::Forms::TextBox^  PWM2;
	private: System::Windows::Forms::Label^  label21;
	private: System::Windows::Forms::HScrollBar^  PWMBar2;
	private: System::Windows::Forms::HScrollBar^  PWMBar1;
	private: System::Windows::Forms::CheckBox^  O1;

	private: System::Windows::Forms::Label^  label22;
private: System::Windows::Forms::ComboBox^  OutputMode1;
private: System::Windows::Forms::ComboBox^  OutputMode2;

private: System::Windows::Forms::Label^  label23;
private: System::Windows::Forms::CheckBox^  O2;
private: System::Windows::Forms::ComboBox^  OutputMode3;

private: System::Windows::Forms::Label^  label24;
private: System::Windows::Forms::CheckBox^  O3;
private: System::Windows::Forms::ComboBox^  OutputMode4;

private: System::Windows::Forms::Label^  label25;
private: System::Windows::Forms::CheckBox^  O4;
private: System::Windows::Forms::ComboBox^  OutputMode5;

private: System::Windows::Forms::Label^  label26;
private: System::Windows::Forms::CheckBox^  O5;
private: System::Windows::Forms::ComboBox^  OutputMode6;

private: System::Windows::Forms::Label^  label27;
private: System::Windows::Forms::CheckBox^  O6;
private: System::Windows::Forms::ComboBox^  OutputMode7;


private: System::Windows::Forms::Label^  label28;
private: System::Windows::Forms::CheckBox^  O7;
private: System::Windows::Forms::ComboBox^  OutputMode8;


private: System::Windows::Forms::Label^  label29;
private: System::Windows::Forms::CheckBox^  O8;
private: System::Windows::Forms::Button^  buttonDisconnect;
private: System::Windows::Forms::Button^  buttonReset;
private: System::ComponentModel::BackgroundWorker^  backgroundWorker1;















	private: System::ComponentModel::IContainer^  components;

	protected: 

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->cardDestination = (gcnew System::Windows::Forms::TextBox());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->buttonConnect = (gcnew System::Windows::Forms::Button());
			this->connectedMessage = (gcnew System::Windows::Forms::Label());
			this->cardPassword = (gcnew System::Windows::Forms::TextBox());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->ADC1 = (gcnew System::Windows::Forms::TextBox());
			this->ADC2 = (gcnew System::Windows::Forms::TextBox());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->I1 = (gcnew System::Windows::Forms::CheckBox());
			this->label5 = (gcnew System::Windows::Forms::Label());
			this->label6 = (gcnew System::Windows::Forms::Label());
			this->Counter1 = (gcnew System::Windows::Forms::TextBox());
			this->label7 = (gcnew System::Windows::Forms::Label());
			this->Debounce1 = (gcnew System::Windows::Forms::TextBox());
			this->SetDebounce1 = (gcnew System::Windows::Forms::Button());
			this->label8 = (gcnew System::Windows::Forms::Label());
			this->I2 = (gcnew System::Windows::Forms::CheckBox());
			this->Counter2 = (gcnew System::Windows::Forms::TextBox());
			this->label9 = (gcnew System::Windows::Forms::Label());
			this->label10 = (gcnew System::Windows::Forms::Label());
			this->Debounce2 = (gcnew System::Windows::Forms::TextBox());
			this->SetDebounce2 = (gcnew System::Windows::Forms::Button());
			this->SetDebounce3 = (gcnew System::Windows::Forms::Button());
			this->Debounce3 = (gcnew System::Windows::Forms::TextBox());
			this->label11 = (gcnew System::Windows::Forms::Label());
			this->Counter3 = (gcnew System::Windows::Forms::TextBox());
			this->label12 = (gcnew System::Windows::Forms::Label());
			this->label13 = (gcnew System::Windows::Forms::Label());
			this->I3 = (gcnew System::Windows::Forms::CheckBox());
			this->SetDebounce4 = (gcnew System::Windows::Forms::Button());
			this->Debounce4 = (gcnew System::Windows::Forms::TextBox());
			this->label14 = (gcnew System::Windows::Forms::Label());
			this->Counter4 = (gcnew System::Windows::Forms::TextBox());
			this->label15 = (gcnew System::Windows::Forms::Label());
			this->label16 = (gcnew System::Windows::Forms::Label());
			this->I4 = (gcnew System::Windows::Forms::CheckBox());
			this->SetDebounce5 = (gcnew System::Windows::Forms::Button());
			this->Debounce5 = (gcnew System::Windows::Forms::TextBox());
			this->label17 = (gcnew System::Windows::Forms::Label());
			this->Counter5 = (gcnew System::Windows::Forms::TextBox());
			this->label18 = (gcnew System::Windows::Forms::Label());
			this->label19 = (gcnew System::Windows::Forms::Label());
			this->I5 = (gcnew System::Windows::Forms::CheckBox());
			this->CounterReset1 = (gcnew System::Windows::Forms::Button());
			this->CounterReset5 = (gcnew System::Windows::Forms::Button());
			this->CounterReset4 = (gcnew System::Windows::Forms::Button());
			this->CounterReset3 = (gcnew System::Windows::Forms::Button());
			this->CounterReset2 = (gcnew System::Windows::Forms::Button());
			this->InputMode1 = (gcnew System::Windows::Forms::ComboBox());
			this->InputMode2 = (gcnew System::Windows::Forms::ComboBox());
			this->InputMode3 = (gcnew System::Windows::Forms::ComboBox());
			this->InputMode4 = (gcnew System::Windows::Forms::ComboBox());
			this->InputMode5 = (gcnew System::Windows::Forms::ComboBox());
			this->ADCBar1 = (gcnew System::Windows::Forms::ProgressBar());
			this->ADCBar2 = (gcnew System::Windows::Forms::ProgressBar());
			this->label20 = (gcnew System::Windows::Forms::Label());
			this->PWM1 = (gcnew System::Windows::Forms::TextBox());
			this->PWM2 = (gcnew System::Windows::Forms::TextBox());
			this->label21 = (gcnew System::Windows::Forms::Label());
			this->PWMBar2 = (gcnew System::Windows::Forms::HScrollBar());
			this->PWMBar1 = (gcnew System::Windows::Forms::HScrollBar());
			this->O1 = (gcnew System::Windows::Forms::CheckBox());
			this->label22 = (gcnew System::Windows::Forms::Label());
			this->OutputMode1 = (gcnew System::Windows::Forms::ComboBox());
			this->OutputMode2 = (gcnew System::Windows::Forms::ComboBox());
			this->label23 = (gcnew System::Windows::Forms::Label());
			this->O2 = (gcnew System::Windows::Forms::CheckBox());
			this->OutputMode3 = (gcnew System::Windows::Forms::ComboBox());
			this->label24 = (gcnew System::Windows::Forms::Label());
			this->O3 = (gcnew System::Windows::Forms::CheckBox());
			this->OutputMode4 = (gcnew System::Windows::Forms::ComboBox());
			this->label25 = (gcnew System::Windows::Forms::Label());
			this->O4 = (gcnew System::Windows::Forms::CheckBox());
			this->OutputMode5 = (gcnew System::Windows::Forms::ComboBox());
			this->label26 = (gcnew System::Windows::Forms::Label());
			this->O5 = (gcnew System::Windows::Forms::CheckBox());
			this->OutputMode6 = (gcnew System::Windows::Forms::ComboBox());
			this->label27 = (gcnew System::Windows::Forms::Label());
			this->O6 = (gcnew System::Windows::Forms::CheckBox());
			this->OutputMode7 = (gcnew System::Windows::Forms::ComboBox());
			this->label28 = (gcnew System::Windows::Forms::Label());
			this->O7 = (gcnew System::Windows::Forms::CheckBox());
			this->OutputMode8 = (gcnew System::Windows::Forms::ComboBox());
			this->label29 = (gcnew System::Windows::Forms::Label());
			this->O8 = (gcnew System::Windows::Forms::CheckBox());
			this->buttonDisconnect = (gcnew System::Windows::Forms::Button());
			this->buttonReset = (gcnew System::Windows::Forms::Button());
			this->backgroundWorker1 = (gcnew System::ComponentModel::BackgroundWorker());
			this->SuspendLayout();
			// 
			// cardDestination
			// 
			this->cardDestination->Location = System::Drawing::Point(104, 17);
			this->cardDestination->Name = L"cardDestination";
			this->cardDestination->Size = System::Drawing::Size(196, 20);
			this->cardDestination->TabIndex = 0;
			this->cardDestination->Text = L"card0";
			this->cardDestination->TextChanged += gcnew System::EventHandler(this, &Form1::cardDestination_TextChanged);
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(12, 20);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(86, 13);
			this->label1->TabIndex = 1;
			this->label1->Text = L"Card destination:";
			// 
			// buttonConnect
			// 
			this->buttonConnect->Location = System::Drawing::Point(306, 15);
			this->buttonConnect->Name = L"buttonConnect";
			this->buttonConnect->Size = System::Drawing::Size(75, 23);
			this->buttonConnect->TabIndex = 2;
			this->buttonConnect->Text = L"Connect";
			this->buttonConnect->UseVisualStyleBackColor = true;
			this->buttonConnect->Click += gcnew System::EventHandler(this, &Form1::buttonConnect_Click);
			// 
			// connectedMessage
			// 
			this->connectedMessage->Location = System::Drawing::Point(387, 20);
			this->connectedMessage->Name = L"connectedMessage";
			this->connectedMessage->Size = System::Drawing::Size(426, 13);
			this->connectedMessage->TabIndex = 3;
			this->connectedMessage->Text = L"Not connected";
			// 
			// cardPassword
			// 
			this->cardPassword->Enabled = false;
			this->cardPassword->Location = System::Drawing::Point(104, 43);
			this->cardPassword->Name = L"cardPassword";
			this->cardPassword->PasswordChar = '*';
			this->cardPassword->Size = System::Drawing::Size(196, 20);
			this->cardPassword->TabIndex = 1;
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(42, 46);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(56, 13);
			this->label2->TabIndex = 5;
			this->label2->Text = L"Password:";
			// 
			// label3
			// 
			this->label3->AutoSize = true;
			this->label3->Location = System::Drawing::Point(12, 96);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(23, 13);
			this->label3->TabIndex = 6;
			this->label3->Text = L"A1:";
			// 
			// ADC1
			// 
			this->ADC1->Enabled = false;
			this->ADC1->Location = System::Drawing::Point(64, 93);
			this->ADC1->Name = L"ADC1";
			this->ADC1->Size = System::Drawing::Size(57, 20);
			this->ADC1->TabIndex = 7;
			this->ADC1->Text = L"\?";
			this->ADC1->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// ADC2
			// 
			this->ADC2->Enabled = false;
			this->ADC2->Location = System::Drawing::Point(64, 119);
			this->ADC2->Name = L"ADC2";
			this->ADC2->Size = System::Drawing::Size(57, 20);
			this->ADC2->TabIndex = 8;
			this->ADC2->Text = L"\?";
			this->ADC2->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// label4
			// 
			this->label4->AutoSize = true;
			this->label4->Location = System::Drawing::Point(12, 122);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(23, 13);
			this->label4->TabIndex = 9;
			this->label4->Text = L"A2:";
			// 
			// I1
			// 
			this->I1->AutoSize = true;
			this->I1->Enabled = false;
			this->I1->Location = System::Drawing::Point(64, 146);
			this->I1->Name = L"I1";
			this->I1->Size = System::Drawing::Size(15, 14);
			this->I1->TabIndex = 10;
			this->I1->UseVisualStyleBackColor = true;
			// 
			// label5
			// 
			this->label5->AutoSize = true;
			this->label5->Location = System::Drawing::Point(12, 146);
			this->label5->Name = L"label5";
			this->label5->Size = System::Drawing::Size(19, 13);
			this->label5->TabIndex = 11;
			this->label5->Text = L"I1:";
			// 
			// label6
			// 
			this->label6->AutoSize = true;
			this->label6->Location = System::Drawing::Point(212, 147);
			this->label6->Name = L"label6";
			this->label6->Size = System::Drawing::Size(47, 13);
			this->label6->TabIndex = 12;
			this->label6->Text = L"Counter:";
			// 
			// Counter1
			// 
			this->Counter1->Enabled = false;
			this->Counter1->Location = System::Drawing::Point(265, 144);
			this->Counter1->Name = L"Counter1";
			this->Counter1->Size = System::Drawing::Size(57, 20);
			this->Counter1->TabIndex = 13;
			this->Counter1->Text = L"\?";
			this->Counter1->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// label7
			// 
			this->label7->AutoSize = true;
			this->label7->Location = System::Drawing::Point(380, 147);
			this->label7->Name = L"label7";
			this->label7->Size = System::Drawing::Size(82, 13);
			this->label7->TabIndex = 14;
			this->label7->Text = L"Debounce (ms):";
			// 
			// Debounce1
			// 
			this->Debounce1->Enabled = false;
			this->Debounce1->Location = System::Drawing::Point(468, 144);
			this->Debounce1->Name = L"Debounce1";
			this->Debounce1->Size = System::Drawing::Size(57, 20);
			this->Debounce1->TabIndex = 15;
			this->Debounce1->Text = L"\?";
			this->Debounce1->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// SetDebounce1
			// 
			this->SetDebounce1->Enabled = false;
			this->SetDebounce1->Location = System::Drawing::Point(531, 142);
			this->SetDebounce1->Name = L"SetDebounce1";
			this->SetDebounce1->Size = System::Drawing::Size(32, 23);
			this->SetDebounce1->TabIndex = 16;
			this->SetDebounce1->Text = L"Set";
			this->SetDebounce1->UseVisualStyleBackColor = true;
			this->SetDebounce1->Click += gcnew System::EventHandler(this, &Form1::SetDebounce1_Click);
			// 
			// label8
			// 
			this->label8->AutoSize = true;
			this->label8->Location = System::Drawing::Point(12, 172);
			this->label8->Name = L"label8";
			this->label8->Size = System::Drawing::Size(19, 13);
			this->label8->TabIndex = 17;
			this->label8->Text = L"I2:";
			// 
			// I2
			// 
			this->I2->AutoSize = true;
			this->I2->Enabled = false;
			this->I2->Location = System::Drawing::Point(64, 172);
			this->I2->Name = L"I2";
			this->I2->Size = System::Drawing::Size(15, 14);
			this->I2->TabIndex = 18;
			this->I2->UseVisualStyleBackColor = true;
			// 
			// Counter2
			// 
			this->Counter2->Enabled = false;
			this->Counter2->Location = System::Drawing::Point(265, 170);
			this->Counter2->Name = L"Counter2";
			this->Counter2->Size = System::Drawing::Size(57, 20);
			this->Counter2->TabIndex = 19;
			this->Counter2->Text = L"\?";
			this->Counter2->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// label9
			// 
			this->label9->AutoSize = true;
			this->label9->Location = System::Drawing::Point(212, 173);
			this->label9->Name = L"label9";
			this->label9->Size = System::Drawing::Size(47, 13);
			this->label9->TabIndex = 20;
			this->label9->Text = L"Counter:";
			// 
			// label10
			// 
			this->label10->AutoSize = true;
			this->label10->Location = System::Drawing::Point(380, 173);
			this->label10->Name = L"label10";
			this->label10->Size = System::Drawing::Size(82, 13);
			this->label10->TabIndex = 21;
			this->label10->Text = L"Debounce (ms):";
			// 
			// Debounce2
			// 
			this->Debounce2->Enabled = false;
			this->Debounce2->Location = System::Drawing::Point(468, 170);
			this->Debounce2->Name = L"Debounce2";
			this->Debounce2->Size = System::Drawing::Size(57, 20);
			this->Debounce2->TabIndex = 22;
			this->Debounce2->Text = L"\?";
			this->Debounce2->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// SetDebounce2
			// 
			this->SetDebounce2->Enabled = false;
			this->SetDebounce2->Location = System::Drawing::Point(531, 168);
			this->SetDebounce2->Name = L"SetDebounce2";
			this->SetDebounce2->Size = System::Drawing::Size(32, 23);
			this->SetDebounce2->TabIndex = 23;
			this->SetDebounce2->Text = L"Set";
			this->SetDebounce2->UseVisualStyleBackColor = true;
			this->SetDebounce2->Click += gcnew System::EventHandler(this, &Form1::SetDebounce2_Click);
			// 
			// SetDebounce3
			// 
			this->SetDebounce3->Enabled = false;
			this->SetDebounce3->Location = System::Drawing::Point(531, 195);
			this->SetDebounce3->Name = L"SetDebounce3";
			this->SetDebounce3->Size = System::Drawing::Size(32, 23);
			this->SetDebounce3->TabIndex = 30;
			this->SetDebounce3->Text = L"Set";
			this->SetDebounce3->UseVisualStyleBackColor = true;
			this->SetDebounce3->Click += gcnew System::EventHandler(this, &Form1::SetDebounce3_Click);
			// 
			// Debounce3
			// 
			this->Debounce3->Enabled = false;
			this->Debounce3->Location = System::Drawing::Point(468, 196);
			this->Debounce3->Name = L"Debounce3";
			this->Debounce3->Size = System::Drawing::Size(57, 20);
			this->Debounce3->TabIndex = 29;
			this->Debounce3->Text = L"\?";
			this->Debounce3->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// label11
			// 
			this->label11->AutoSize = true;
			this->label11->Location = System::Drawing::Point(380, 199);
			this->label11->Name = L"label11";
			this->label11->Size = System::Drawing::Size(82, 13);
			this->label11->TabIndex = 28;
			this->label11->Text = L"Debounce (ms):";
			// 
			// Counter3
			// 
			this->Counter3->Enabled = false;
			this->Counter3->Location = System::Drawing::Point(265, 196);
			this->Counter3->Name = L"Counter3";
			this->Counter3->Size = System::Drawing::Size(57, 20);
			this->Counter3->TabIndex = 27;
			this->Counter3->Text = L"\?";
			this->Counter3->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// label12
			// 
			this->label12->AutoSize = true;
			this->label12->Location = System::Drawing::Point(212, 199);
			this->label12->Name = L"label12";
			this->label12->Size = System::Drawing::Size(47, 13);
			this->label12->TabIndex = 26;
			this->label12->Text = L"Counter:";
			// 
			// label13
			// 
			this->label13->AutoSize = true;
			this->label13->Location = System::Drawing::Point(12, 199);
			this->label13->Name = L"label13";
			this->label13->Size = System::Drawing::Size(19, 13);
			this->label13->TabIndex = 25;
			this->label13->Text = L"I3:";
			// 
			// I3
			// 
			this->I3->AutoSize = true;
			this->I3->Enabled = false;
			this->I3->Location = System::Drawing::Point(64, 199);
			this->I3->Name = L"I3";
			this->I3->Size = System::Drawing::Size(15, 14);
			this->I3->TabIndex = 24;
			this->I3->UseVisualStyleBackColor = true;
			// 
			// SetDebounce4
			// 
			this->SetDebounce4->Enabled = false;
			this->SetDebounce4->Location = System::Drawing::Point(531, 220);
			this->SetDebounce4->Name = L"SetDebounce4";
			this->SetDebounce4->Size = System::Drawing::Size(32, 23);
			this->SetDebounce4->TabIndex = 37;
			this->SetDebounce4->Text = L"Set";
			this->SetDebounce4->UseVisualStyleBackColor = true;
			this->SetDebounce4->Click += gcnew System::EventHandler(this, &Form1::SetDebounce4_Click);
			// 
			// Debounce4
			// 
			this->Debounce4->Enabled = false;
			this->Debounce4->Location = System::Drawing::Point(468, 222);
			this->Debounce4->Name = L"Debounce4";
			this->Debounce4->Size = System::Drawing::Size(57, 20);
			this->Debounce4->TabIndex = 36;
			this->Debounce4->Text = L"\?";
			this->Debounce4->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// label14
			// 
			this->label14->AutoSize = true;
			this->label14->Location = System::Drawing::Point(380, 225);
			this->label14->Name = L"label14";
			this->label14->Size = System::Drawing::Size(82, 13);
			this->label14->TabIndex = 35;
			this->label14->Text = L"Debounce (ms):";
			// 
			// Counter4
			// 
			this->Counter4->Enabled = false;
			this->Counter4->Location = System::Drawing::Point(265, 222);
			this->Counter4->Name = L"Counter4";
			this->Counter4->Size = System::Drawing::Size(57, 20);
			this->Counter4->TabIndex = 34;
			this->Counter4->Text = L"\?";
			this->Counter4->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// label15
			// 
			this->label15->AutoSize = true;
			this->label15->Location = System::Drawing::Point(212, 225);
			this->label15->Name = L"label15";
			this->label15->Size = System::Drawing::Size(47, 13);
			this->label15->TabIndex = 33;
			this->label15->Text = L"Counter:";
			// 
			// label16
			// 
			this->label16->AutoSize = true;
			this->label16->Location = System::Drawing::Point(12, 224);
			this->label16->Name = L"label16";
			this->label16->Size = System::Drawing::Size(19, 13);
			this->label16->TabIndex = 32;
			this->label16->Text = L"I4:";
			// 
			// I4
			// 
			this->I4->AutoSize = true;
			this->I4->Enabled = false;
			this->I4->Location = System::Drawing::Point(64, 224);
			this->I4->Name = L"I4";
			this->I4->Size = System::Drawing::Size(15, 14);
			this->I4->TabIndex = 31;
			this->I4->UseVisualStyleBackColor = true;
			// 
			// SetDebounce5
			// 
			this->SetDebounce5->Enabled = false;
			this->SetDebounce5->Location = System::Drawing::Point(531, 246);
			this->SetDebounce5->Name = L"SetDebounce5";
			this->SetDebounce5->Size = System::Drawing::Size(32, 23);
			this->SetDebounce5->TabIndex = 44;
			this->SetDebounce5->Text = L"Set";
			this->SetDebounce5->UseVisualStyleBackColor = true;
			this->SetDebounce5->Click += gcnew System::EventHandler(this, &Form1::SetDebounce5_Click);
			// 
			// Debounce5
			// 
			this->Debounce5->Enabled = false;
			this->Debounce5->Location = System::Drawing::Point(468, 248);
			this->Debounce5->Name = L"Debounce5";
			this->Debounce5->Size = System::Drawing::Size(57, 20);
			this->Debounce5->TabIndex = 43;
			this->Debounce5->Text = L"\?";
			this->Debounce5->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// label17
			// 
			this->label17->AutoSize = true;
			this->label17->Location = System::Drawing::Point(380, 251);
			this->label17->Name = L"label17";
			this->label17->Size = System::Drawing::Size(82, 13);
			this->label17->TabIndex = 42;
			this->label17->Text = L"Debounce (ms):";
			// 
			// Counter5
			// 
			this->Counter5->Enabled = false;
			this->Counter5->Location = System::Drawing::Point(265, 248);
			this->Counter5->Name = L"Counter5";
			this->Counter5->Size = System::Drawing::Size(57, 20);
			this->Counter5->TabIndex = 41;
			this->Counter5->Text = L"\?";
			this->Counter5->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// label18
			// 
			this->label18->AutoSize = true;
			this->label18->Location = System::Drawing::Point(212, 251);
			this->label18->Name = L"label18";
			this->label18->Size = System::Drawing::Size(47, 13);
			this->label18->TabIndex = 40;
			this->label18->Text = L"Counter:";
			// 
			// label19
			// 
			this->label19->AutoSize = true;
			this->label19->Location = System::Drawing::Point(12, 250);
			this->label19->Name = L"label19";
			this->label19->Size = System::Drawing::Size(19, 13);
			this->label19->TabIndex = 39;
			this->label19->Text = L"I5:";
			// 
			// I5
			// 
			this->I5->AutoSize = true;
			this->I5->Enabled = false;
			this->I5->Location = System::Drawing::Point(64, 250);
			this->I5->Name = L"I5";
			this->I5->Size = System::Drawing::Size(15, 14);
			this->I5->TabIndex = 38;
			this->I5->UseVisualStyleBackColor = true;
			// 
			// CounterReset1
			// 
			this->CounterReset1->Enabled = false;
			this->CounterReset1->Location = System::Drawing::Point(328, 142);
			this->CounterReset1->Name = L"CounterReset1";
			this->CounterReset1->Size = System::Drawing::Size(46, 23);
			this->CounterReset1->TabIndex = 45;
			this->CounterReset1->Text = L"Reset";
			this->CounterReset1->UseVisualStyleBackColor = true;
			this->CounterReset1->Click += gcnew System::EventHandler(this, &Form1::CounterReset1_Click);
			// 
			// CounterReset5
			// 
			this->CounterReset5->Enabled = false;
			this->CounterReset5->Location = System::Drawing::Point(328, 246);
			this->CounterReset5->Name = L"CounterReset5";
			this->CounterReset5->Size = System::Drawing::Size(46, 23);
			this->CounterReset5->TabIndex = 46;
			this->CounterReset5->Text = L"Reset";
			this->CounterReset5->UseVisualStyleBackColor = true;
			this->CounterReset5->Click += gcnew System::EventHandler(this, &Form1::CounterReset5_Click);
			// 
			// CounterReset4
			// 
			this->CounterReset4->Enabled = false;
			this->CounterReset4->Location = System::Drawing::Point(328, 220);
			this->CounterReset4->Name = L"CounterReset4";
			this->CounterReset4->Size = System::Drawing::Size(46, 23);
			this->CounterReset4->TabIndex = 47;
			this->CounterReset4->Text = L"Reset";
			this->CounterReset4->UseVisualStyleBackColor = true;
			this->CounterReset4->Click += gcnew System::EventHandler(this, &Form1::CounterReset4_Click);
			// 
			// CounterReset3
			// 
			this->CounterReset3->Enabled = false;
			this->CounterReset3->Location = System::Drawing::Point(328, 195);
			this->CounterReset3->Name = L"CounterReset3";
			this->CounterReset3->Size = System::Drawing::Size(46, 23);
			this->CounterReset3->TabIndex = 48;
			this->CounterReset3->Text = L"Reset";
			this->CounterReset3->UseVisualStyleBackColor = true;
			this->CounterReset3->Click += gcnew System::EventHandler(this, &Form1::CounterReset3_Click);
			// 
			// CounterReset2
			// 
			this->CounterReset2->Enabled = false;
			this->CounterReset2->Location = System::Drawing::Point(328, 168);
			this->CounterReset2->Name = L"CounterReset2";
			this->CounterReset2->Size = System::Drawing::Size(46, 23);
			this->CounterReset2->TabIndex = 49;
			this->CounterReset2->Text = L"Reset";
			this->CounterReset2->UseVisualStyleBackColor = true;
			this->CounterReset2->Click += gcnew System::EventHandler(this, &Form1::CounterReset2_Click);
			// 
			// InputMode1
			// 
			this->InputMode1->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->InputMode1->Enabled = false;
			this->InputMode1->FormattingEnabled = true;
			this->InputMode1->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"Mode Normal", L"Mode Frequency"});
			this->InputMode1->Location = System::Drawing::Point(85, 143);
			this->InputMode1->Name = L"InputMode1";
			this->InputMode1->Size = System::Drawing::Size(121, 21);
			this->InputMode1->TabIndex = 50;
			this->InputMode1->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::InputMode1_SelectedIndexChanged);
			// 
			// InputMode2
			// 
			this->InputMode2->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->InputMode2->Enabled = false;
			this->InputMode2->FormattingEnabled = true;
			this->InputMode2->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"Mode Normal", L"Mode Frequency"});
			this->InputMode2->Location = System::Drawing::Point(85, 169);
			this->InputMode2->Name = L"InputMode2";
			this->InputMode2->Size = System::Drawing::Size(121, 21);
			this->InputMode2->TabIndex = 51;
			this->InputMode2->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::InputMode2_SelectedIndexChanged);
			// 
			// InputMode3
			// 
			this->InputMode3->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->InputMode3->Enabled = false;
			this->InputMode3->FormattingEnabled = true;
			this->InputMode3->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"Mode Normal", L"Mode Frequency"});
			this->InputMode3->Location = System::Drawing::Point(85, 196);
			this->InputMode3->Name = L"InputMode3";
			this->InputMode3->Size = System::Drawing::Size(121, 21);
			this->InputMode3->TabIndex = 52;
			this->InputMode3->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::InputMode3_SelectedIndexChanged);
			// 
			// InputMode4
			// 
			this->InputMode4->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->InputMode4->Enabled = false;
			this->InputMode4->FormattingEnabled = true;
			this->InputMode4->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"Mode Normal", L"Mode Frequency"});
			this->InputMode4->Location = System::Drawing::Point(85, 220);
			this->InputMode4->Name = L"InputMode4";
			this->InputMode4->Size = System::Drawing::Size(121, 21);
			this->InputMode4->TabIndex = 53;
			this->InputMode4->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::InputMode4_SelectedIndexChanged);
			// 
			// InputMode5
			// 
			this->InputMode5->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->InputMode5->Enabled = false;
			this->InputMode5->FormattingEnabled = true;
			this->InputMode5->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"Mode Normal", L"Mode Frequency"});
			this->InputMode5->Location = System::Drawing::Point(85, 246);
			this->InputMode5->Name = L"InputMode5";
			this->InputMode5->Size = System::Drawing::Size(121, 21);
			this->InputMode5->TabIndex = 54;
			this->InputMode5->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::InputMode5_SelectedIndexChanged);
			// 
			// ADCBar1
			// 
			this->ADCBar1->Location = System::Drawing::Point(138, 93);
			this->ADCBar1->Maximum = 1023;
			this->ADCBar1->Name = L"ADCBar1";
			this->ADCBar1->Size = System::Drawing::Size(512, 20);
			this->ADCBar1->Style = System::Windows::Forms::ProgressBarStyle::Continuous;
			this->ADCBar1->TabIndex = 55;
			// 
			// ADCBar2
			// 
			this->ADCBar2->Location = System::Drawing::Point(138, 119);
			this->ADCBar2->Maximum = 1023;
			this->ADCBar2->Name = L"ADCBar2";
			this->ADCBar2->Size = System::Drawing::Size(512, 20);
			this->ADCBar2->Style = System::Windows::Forms::ProgressBarStyle::Continuous;
			this->ADCBar2->TabIndex = 56;
			// 
			// label20
			// 
			this->label20->AutoSize = true;
			this->label20->Location = System::Drawing::Point(12, 275);
			this->label20->Name = L"label20";
			this->label20->Size = System::Drawing::Size(43, 13);
			this->label20->TabIndex = 57;
			this->label20->Text = L"PWM1:";
			// 
			// PWM1
			// 
			this->PWM1->Enabled = false;
			this->PWM1->Location = System::Drawing::Point(64, 272);
			this->PWM1->Name = L"PWM1";
			this->PWM1->Size = System::Drawing::Size(57, 20);
			this->PWM1->TabIndex = 58;
			this->PWM1->Text = L"\?";
			this->PWM1->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// PWM2
			// 
			this->PWM2->Enabled = false;
			this->PWM2->Location = System::Drawing::Point(64, 301);
			this->PWM2->Name = L"PWM2";
			this->PWM2->Size = System::Drawing::Size(57, 20);
			this->PWM2->TabIndex = 61;
			this->PWM2->Text = L"\?";
			this->PWM2->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// label21
			// 
			this->label21->AutoSize = true;
			this->label21->Location = System::Drawing::Point(12, 304);
			this->label21->Name = L"label21";
			this->label21->Size = System::Drawing::Size(43, 13);
			this->label21->TabIndex = 60;
			this->label21->Text = L"PWM2:";
			// 
			// PWMBar2
			// 
			this->PWMBar2->Enabled = false;
			this->PWMBar2->Location = System::Drawing::Point(138, 301);
			this->PWMBar2->Maximum = 1032;
			this->PWMBar2->Name = L"PWMBar2";
			this->PWMBar2->Size = System::Drawing::Size(512, 20);
			this->PWMBar2->TabIndex = 62;
			this->PWMBar2->TabStop = true;
			this->PWMBar2->Scroll += gcnew System::Windows::Forms::ScrollEventHandler(this, &Form1::PWMBar2_Scroll);
			// 
			// PWMBar1
			// 
			this->PWMBar1->Enabled = false;
			this->PWMBar1->Location = System::Drawing::Point(138, 272);
			this->PWMBar1->Maximum = 1032;
			this->PWMBar1->Name = L"PWMBar1";
			this->PWMBar1->Size = System::Drawing::Size(512, 20);
			this->PWMBar1->TabIndex = 63;
			this->PWMBar1->TabStop = true;
			this->PWMBar1->Scroll += gcnew System::Windows::Forms::ScrollEventHandler(this, &Form1::PWMBar1_Scroll);
			// 
			// O1
			// 
			this->O1->AutoSize = true;
			this->O1->Enabled = false;
			this->O1->Location = System::Drawing::Point(64, 327);
			this->O1->Name = L"O1";
			this->O1->Size = System::Drawing::Size(15, 14);
			this->O1->TabIndex = 64;
			this->O1->UseVisualStyleBackColor = true;
			this->O1->CheckedChanged += gcnew System::EventHandler(this, &Form1::O1_CheckedChanged);
			// 
			// label22
			// 
			this->label22->AutoSize = true;
			this->label22->Location = System::Drawing::Point(12, 327);
			this->label22->Name = L"label22";
			this->label22->Size = System::Drawing::Size(24, 13);
			this->label22->TabIndex = 65;
			this->label22->Text = L"O1:";
			// 
			// OutputMode1
			// 
			this->OutputMode1->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->OutputMode1->Enabled = false;
			this->OutputMode1->FormattingEnabled = true;
			this->OutputMode1->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"Mode Normal", L"Mode Servo"});
			this->OutputMode1->Location = System::Drawing::Point(85, 324);
			this->OutputMode1->Name = L"OutputMode1";
			this->OutputMode1->Size = System::Drawing::Size(121, 21);
			this->OutputMode1->TabIndex = 66;
			// 
			// OutputMode2
			// 
			this->OutputMode2->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->OutputMode2->Enabled = false;
			this->OutputMode2->FormattingEnabled = true;
			this->OutputMode2->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"Mode Normal", L"Mode Servo"});
			this->OutputMode2->Location = System::Drawing::Point(85, 351);
			this->OutputMode2->Name = L"OutputMode2";
			this->OutputMode2->Size = System::Drawing::Size(121, 21);
			this->OutputMode2->TabIndex = 69;
			// 
			// label23
			// 
			this->label23->AutoSize = true;
			this->label23->Location = System::Drawing::Point(12, 354);
			this->label23->Name = L"label23";
			this->label23->Size = System::Drawing::Size(24, 13);
			this->label23->TabIndex = 68;
			this->label23->Text = L"O2:";
			// 
			// O2
			// 
			this->O2->AutoSize = true;
			this->O2->Enabled = false;
			this->O2->Location = System::Drawing::Point(64, 354);
			this->O2->Name = L"O2";
			this->O2->Size = System::Drawing::Size(15, 14);
			this->O2->TabIndex = 67;
			this->O2->UseVisualStyleBackColor = true;
			this->O2->CheckedChanged += gcnew System::EventHandler(this, &Form1::O2_CheckedChanged);
			// 
			// OutputMode3
			// 
			this->OutputMode3->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->OutputMode3->Enabled = false;
			this->OutputMode3->FormattingEnabled = true;
			this->OutputMode3->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"Mode Normal", L"Mode Servo"});
			this->OutputMode3->Location = System::Drawing::Point(85, 378);
			this->OutputMode3->Name = L"OutputMode3";
			this->OutputMode3->Size = System::Drawing::Size(121, 21);
			this->OutputMode3->TabIndex = 72;
			// 
			// label24
			// 
			this->label24->AutoSize = true;
			this->label24->Location = System::Drawing::Point(12, 381);
			this->label24->Name = L"label24";
			this->label24->Size = System::Drawing::Size(24, 13);
			this->label24->TabIndex = 71;
			this->label24->Text = L"O3:";
			// 
			// O3
			// 
			this->O3->AutoSize = true;
			this->O3->Enabled = false;
			this->O3->Location = System::Drawing::Point(64, 381);
			this->O3->Name = L"O3";
			this->O3->Size = System::Drawing::Size(15, 14);
			this->O3->TabIndex = 70;
			this->O3->UseVisualStyleBackColor = true;
			this->O3->CheckedChanged += gcnew System::EventHandler(this, &Form1::O3_CheckedChanged);
			// 
			// OutputMode4
			// 
			this->OutputMode4->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->OutputMode4->Enabled = false;
			this->OutputMode4->FormattingEnabled = true;
			this->OutputMode4->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"Mode Normal", L"Mode Servo"});
			this->OutputMode4->Location = System::Drawing::Point(85, 405);
			this->OutputMode4->Name = L"OutputMode4";
			this->OutputMode4->Size = System::Drawing::Size(121, 21);
			this->OutputMode4->TabIndex = 75;
			// 
			// label25
			// 
			this->label25->AutoSize = true;
			this->label25->Location = System::Drawing::Point(12, 408);
			this->label25->Name = L"label25";
			this->label25->Size = System::Drawing::Size(24, 13);
			this->label25->TabIndex = 74;
			this->label25->Text = L"O4:";
			// 
			// O4
			// 
			this->O4->AutoSize = true;
			this->O4->Enabled = false;
			this->O4->Location = System::Drawing::Point(64, 408);
			this->O4->Name = L"O4";
			this->O4->Size = System::Drawing::Size(15, 14);
			this->O4->TabIndex = 73;
			this->O4->UseVisualStyleBackColor = true;
			this->O4->CheckedChanged += gcnew System::EventHandler(this, &Form1::O4_CheckedChanged);
			// 
			// OutputMode5
			// 
			this->OutputMode5->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->OutputMode5->Enabled = false;
			this->OutputMode5->FormattingEnabled = true;
			this->OutputMode5->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"Mode Normal", L"Mode Servo"});
			this->OutputMode5->Location = System::Drawing::Point(85, 432);
			this->OutputMode5->Name = L"OutputMode5";
			this->OutputMode5->Size = System::Drawing::Size(121, 21);
			this->OutputMode5->TabIndex = 78;
			// 
			// label26
			// 
			this->label26->AutoSize = true;
			this->label26->Location = System::Drawing::Point(12, 435);
			this->label26->Name = L"label26";
			this->label26->Size = System::Drawing::Size(24, 13);
			this->label26->TabIndex = 77;
			this->label26->Text = L"O5:";
			// 
			// O5
			// 
			this->O5->AutoSize = true;
			this->O5->Enabled = false;
			this->O5->Location = System::Drawing::Point(64, 435);
			this->O5->Name = L"O5";
			this->O5->Size = System::Drawing::Size(15, 14);
			this->O5->TabIndex = 76;
			this->O5->UseVisualStyleBackColor = true;
			this->O5->CheckedChanged += gcnew System::EventHandler(this, &Form1::O5_CheckedChanged);
			// 
			// OutputMode6
			// 
			this->OutputMode6->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->OutputMode6->Enabled = false;
			this->OutputMode6->FormattingEnabled = true;
			this->OutputMode6->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"Mode Normal", L"Mode Servo"});
			this->OutputMode6->Location = System::Drawing::Point(85, 459);
			this->OutputMode6->Name = L"OutputMode6";
			this->OutputMode6->Size = System::Drawing::Size(121, 21);
			this->OutputMode6->TabIndex = 81;
			// 
			// label27
			// 
			this->label27->AutoSize = true;
			this->label27->Location = System::Drawing::Point(12, 462);
			this->label27->Name = L"label27";
			this->label27->Size = System::Drawing::Size(24, 13);
			this->label27->TabIndex = 80;
			this->label27->Text = L"O6:";
			// 
			// O6
			// 
			this->O6->AutoSize = true;
			this->O6->Enabled = false;
			this->O6->Location = System::Drawing::Point(64, 462);
			this->O6->Name = L"O6";
			this->O6->Size = System::Drawing::Size(15, 14);
			this->O6->TabIndex = 79;
			this->O6->UseVisualStyleBackColor = true;
			this->O6->CheckedChanged += gcnew System::EventHandler(this, &Form1::O6_CheckedChanged);
			// 
			// OutputMode7
			// 
			this->OutputMode7->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->OutputMode7->Enabled = false;
			this->OutputMode7->FormattingEnabled = true;
			this->OutputMode7->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"Mode Normal", L"Mode Servo"});
			this->OutputMode7->Location = System::Drawing::Point(85, 486);
			this->OutputMode7->Name = L"OutputMode7";
			this->OutputMode7->Size = System::Drawing::Size(121, 21);
			this->OutputMode7->TabIndex = 84;
			// 
			// label28
			// 
			this->label28->AutoSize = true;
			this->label28->Location = System::Drawing::Point(12, 489);
			this->label28->Name = L"label28";
			this->label28->Size = System::Drawing::Size(24, 13);
			this->label28->TabIndex = 83;
			this->label28->Text = L"O7:";
			// 
			// O7
			// 
			this->O7->AutoSize = true;
			this->O7->Enabled = false;
			this->O7->Location = System::Drawing::Point(64, 489);
			this->O7->Name = L"O7";
			this->O7->Size = System::Drawing::Size(15, 14);
			this->O7->TabIndex = 82;
			this->O7->UseVisualStyleBackColor = true;
			this->O7->CheckedChanged += gcnew System::EventHandler(this, &Form1::O7_CheckedChanged);
			// 
			// OutputMode8
			// 
			this->OutputMode8->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->OutputMode8->Enabled = false;
			this->OutputMode8->FormattingEnabled = true;
			this->OutputMode8->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"Mode Normal", L"Mode Servo"});
			this->OutputMode8->Location = System::Drawing::Point(85, 513);
			this->OutputMode8->Name = L"OutputMode8";
			this->OutputMode8->Size = System::Drawing::Size(121, 21);
			this->OutputMode8->TabIndex = 87;
			// 
			// label29
			// 
			this->label29->AutoSize = true;
			this->label29->Location = System::Drawing::Point(12, 516);
			this->label29->Name = L"label29";
			this->label29->Size = System::Drawing::Size(24, 13);
			this->label29->TabIndex = 86;
			this->label29->Text = L"O8:";
			// 
			// O8
			// 
			this->O8->AutoSize = true;
			this->O8->Enabled = false;
			this->O8->Location = System::Drawing::Point(64, 516);
			this->O8->Name = L"O8";
			this->O8->Size = System::Drawing::Size(15, 14);
			this->O8->TabIndex = 85;
			this->O8->UseVisualStyleBackColor = true;
			this->O8->CheckedChanged += gcnew System::EventHandler(this, &Form1::O8_CheckedChanged);
			// 
			// buttonDisconnect
			// 
			this->buttonDisconnect->Enabled = false;
			this->buttonDisconnect->Location = System::Drawing::Point(306, 41);
			this->buttonDisconnect->Name = L"buttonDisconnect";
			this->buttonDisconnect->Size = System::Drawing::Size(75, 23);
			this->buttonDisconnect->TabIndex = 88;
			this->buttonDisconnect->Text = L"Disconnect";
			this->buttonDisconnect->UseVisualStyleBackColor = true;
			this->buttonDisconnect->Click += gcnew System::EventHandler(this, &Form1::buttonDisconnect_Click);
			// 
			// buttonReset
			// 
			this->buttonReset->Enabled = false;
			this->buttonReset->Location = System::Drawing::Point(387, 41);
			this->buttonReset->Name = L"buttonReset";
			this->buttonReset->Size = System::Drawing::Size(75, 23);
			this->buttonReset->TabIndex = 89;
			this->buttonReset->Text = L"Reset Card";
			this->buttonReset->UseVisualStyleBackColor = true;
			this->buttonReset->Click += gcnew System::EventHandler(this, &Form1::buttonReset_Click);
			// 
			// backgroundWorker1
			// 
			this->backgroundWorker1->WorkerReportsProgress = true;
			this->backgroundWorker1->DoWork += gcnew System::ComponentModel::DoWorkEventHandler(this, &Form1::backgroundWorker1_DoWork);
			this->backgroundWorker1->ProgressChanged += gcnew System::ComponentModel::ProgressChangedEventHandler(this, &Form1::backgroundWorker1_ProgressChanged);
			// 
			// Form1
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(892, 553);
			this->Controls->Add(this->buttonReset);
			this->Controls->Add(this->buttonDisconnect);
			this->Controls->Add(this->OutputMode8);
			this->Controls->Add(this->label29);
			this->Controls->Add(this->O8);
			this->Controls->Add(this->OutputMode7);
			this->Controls->Add(this->label28);
			this->Controls->Add(this->O7);
			this->Controls->Add(this->OutputMode6);
			this->Controls->Add(this->label27);
			this->Controls->Add(this->O6);
			this->Controls->Add(this->OutputMode5);
			this->Controls->Add(this->label26);
			this->Controls->Add(this->O5);
			this->Controls->Add(this->OutputMode4);
			this->Controls->Add(this->label25);
			this->Controls->Add(this->O4);
			this->Controls->Add(this->OutputMode3);
			this->Controls->Add(this->label24);
			this->Controls->Add(this->O3);
			this->Controls->Add(this->OutputMode2);
			this->Controls->Add(this->label23);
			this->Controls->Add(this->O2);
			this->Controls->Add(this->OutputMode1);
			this->Controls->Add(this->label22);
			this->Controls->Add(this->O1);
			this->Controls->Add(this->PWMBar1);
			this->Controls->Add(this->PWMBar2);
			this->Controls->Add(this->PWM2);
			this->Controls->Add(this->label21);
			this->Controls->Add(this->PWM1);
			this->Controls->Add(this->label20);
			this->Controls->Add(this->ADCBar2);
			this->Controls->Add(this->ADCBar1);
			this->Controls->Add(this->InputMode5);
			this->Controls->Add(this->InputMode4);
			this->Controls->Add(this->InputMode3);
			this->Controls->Add(this->InputMode2);
			this->Controls->Add(this->InputMode1);
			this->Controls->Add(this->CounterReset2);
			this->Controls->Add(this->CounterReset3);
			this->Controls->Add(this->CounterReset4);
			this->Controls->Add(this->CounterReset5);
			this->Controls->Add(this->CounterReset1);
			this->Controls->Add(this->SetDebounce5);
			this->Controls->Add(this->Debounce5);
			this->Controls->Add(this->label17);
			this->Controls->Add(this->Counter5);
			this->Controls->Add(this->label18);
			this->Controls->Add(this->label19);
			this->Controls->Add(this->I5);
			this->Controls->Add(this->SetDebounce4);
			this->Controls->Add(this->Debounce4);
			this->Controls->Add(this->label14);
			this->Controls->Add(this->Counter4);
			this->Controls->Add(this->label15);
			this->Controls->Add(this->label16);
			this->Controls->Add(this->I4);
			this->Controls->Add(this->SetDebounce3);
			this->Controls->Add(this->Debounce3);
			this->Controls->Add(this->label11);
			this->Controls->Add(this->Counter3);
			this->Controls->Add(this->label12);
			this->Controls->Add(this->label13);
			this->Controls->Add(this->I3);
			this->Controls->Add(this->SetDebounce2);
			this->Controls->Add(this->Debounce2);
			this->Controls->Add(this->label10);
			this->Controls->Add(this->label9);
			this->Controls->Add(this->Counter2);
			this->Controls->Add(this->I2);
			this->Controls->Add(this->label8);
			this->Controls->Add(this->SetDebounce1);
			this->Controls->Add(this->Debounce1);
			this->Controls->Add(this->label7);
			this->Controls->Add(this->Counter1);
			this->Controls->Add(this->label6);
			this->Controls->Add(this->label5);
			this->Controls->Add(this->I1);
			this->Controls->Add(this->label4);
			this->Controls->Add(this->ADC2);
			this->Controls->Add(this->ADC1);
			this->Controls->Add(this->label3);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->cardPassword);
			this->Controls->Add(this->connectedMessage);
			this->Controls->Add(this->buttonConnect);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->cardDestination);
			this->Name = L"Form1";
			this->Text = L"Open8055Demo";
			this->Load += gcnew System::EventHandler(this, &Form1::Form1_Load);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

	static int					cardHandle = -1;

	private: int modeInputToIndex(int mode) {
				 switch (mode)
				 {
					 case OPEN8055_MODE_INPUT:
						 return 0;
					 case OPEN8055_MODE_FREQUENCY:
						 return 1;
					 default:
						 return -1;
				 }
			 }

	private: int modeInputFromIndex(int index) {
				 switch (index)
				 {
					 case 0:
						 return OPEN8055_MODE_INPUT;
					 case 1:
						 return OPEN8055_MODE_FREQUENCY;
					 default:
						 return -1;
				 }
			 }

	private: int modeOutputToIndex(int mode) {
				 switch (mode)
				 {
					 case OPEN8055_MODE_OUTPUT:
						 return 0;
					 case OPEN8055_MODE_SERVO:
						 return 1;
					 case OPEN8055_MODE_ISERVO:
						 return 2;
					 default:
						 return -1;
				 }
			 }

	private: int modeOutputFromIndex(int index) {
				 switch (index)
				 {
					 case 0:
						 return OPEN8055_MODE_OUTPUT;
					 case 1:
						 return OPEN8055_MODE_SERVO;
					 case 2: 
						 return OPEN8055_MODE_ISERVO;
					 default:
						 return -1;
				 }
			 }

	private: System::Void updateAllConfig(void) {
				 InputMode1->SelectedIndex = modeInputToIndex(Open8055_GetModeInput(cardHandle, 0));
				 InputMode2->SelectedIndex = modeInputToIndex(Open8055_GetModeInput(cardHandle, 1));
				 InputMode3->SelectedIndex = modeInputToIndex(Open8055_GetModeInput(cardHandle, 2));
				 InputMode4->SelectedIndex = modeInputToIndex(Open8055_GetModeInput(cardHandle, 3));
				 InputMode5->SelectedIndex = modeInputToIndex(Open8055_GetModeInput(cardHandle, 4));

				 OutputMode1->SelectedIndex = modeOutputToIndex(Open8055_GetModeOutput(cardHandle, 0));
				 OutputMode2->SelectedIndex = modeOutputToIndex(Open8055_GetModeOutput(cardHandle, 1));
				 OutputMode3->SelectedIndex = modeOutputToIndex(Open8055_GetModeOutput(cardHandle, 2));
				 OutputMode4->SelectedIndex = modeOutputToIndex(Open8055_GetModeOutput(cardHandle, 3));
				 OutputMode5->SelectedIndex = modeOutputToIndex(Open8055_GetModeOutput(cardHandle, 4));
				 OutputMode6->SelectedIndex = modeOutputToIndex(Open8055_GetModeOutput(cardHandle, 5));
				 OutputMode7->SelectedIndex = modeOutputToIndex(Open8055_GetModeOutput(cardHandle, 6));
				 OutputMode8->SelectedIndex = modeOutputToIndex(Open8055_GetModeOutput(cardHandle, 7));
			 }

	private: System::Void Form1_Load(System::Object^  sender, System::EventArgs^  e) {
			 }
	private: System::Void Form1_Destroy(void) {
				 if (cardHandle >= 0)
					 Open8055_Close(cardHandle);
			 }
	private: System::Void cardDestination_TextChanged(System::Object^  sender, System::EventArgs^  e) {
			 }
	private: System::Void buttonConnect_Click(System::Object^  sender, System::EventArgs^  e) {
				 // Convert the card destination and password into C strings
				 char *destination = (char *)(void *)Marshal::StringToHGlobalAnsi(cardDestination->Text);
				 char *password = (char *)(void *)Marshal::StringToHGlobalAnsi(cardPassword->Text);

				 // Close any previously open card
				 if (cardHandle >= 0)
				 {
					 buttonDisconnect_Click(sender, e);
				 }
					
				 // Try to open the requested card
				 cardHandle = Open8055_Connect(destination, password);

				 // Free the C strings
				 Marshal::FreeHGlobal((System::IntPtr)destination);
				 Marshal::FreeHGlobal((System::IntPtr)password);
				 
				 // See if we actually found the requested card
				 if (cardHandle < 0)
				 {
					 connectedMessage->Text = "Connection to '" + cardDestination->Text +
						 "' failed: " + 
						 Marshal::PtrToStringAnsi((System::IntPtr)Open8055_LastError(NULL));
				 } else {
					 connectedMessage->Text = "Connected to '" + cardDestination->Text + "'";
					 backgroundWorker1->RunWorkerAsync();

					 Debounce1->Text = Open8055_GetDebounce(cardHandle, 0).ToString("f1");
					 Debounce2->Text = Open8055_GetDebounce(cardHandle, 1).ToString("f1");
					 Debounce3->Text = Open8055_GetDebounce(cardHandle, 2).ToString("f1");
					 Debounce4->Text = Open8055_GetDebounce(cardHandle, 3).ToString("f1");
					 Debounce5->Text = Open8055_GetDebounce(cardHandle, 4).ToString("f1");

					 PWM1->Text = Open8055_GetPWM(cardHandle, 0).ToString("d");
					 PWMBar1->Value = Open8055_GetPWM(cardHandle, 0);
					 PWM2->Text = Open8055_GetPWM(cardHandle, 1).ToString("d");
					 PWMBar2->Value = Open8055_GetPWM(cardHandle, 1);

					 O1->Checked = Open8055_GetOutput(cardHandle, 0) != 0;
					 O2->Checked = Open8055_GetOutput(cardHandle, 1) != 0;
					 O3->Checked = Open8055_GetOutput(cardHandle, 2) != 0;
					 O4->Checked = Open8055_GetOutput(cardHandle, 3) != 0;
					 O5->Checked = Open8055_GetOutput(cardHandle, 4) != 0;
					 O6->Checked = Open8055_GetOutput(cardHandle, 5) != 0;
					 O7->Checked = Open8055_GetOutput(cardHandle, 6) != 0;
					 O8->Checked = Open8055_GetOutput(cardHandle, 7) != 0;

					 buttonDisconnect->Enabled = true;
					 buttonReset->Enabled = true;

					 InputMode1->Enabled = true; 
					 CounterReset1->Enabled = true; 
					 Debounce1->Enabled = true; SetDebounce1->Enabled = true;

					 InputMode2->Enabled = true; 
					 CounterReset2->Enabled = true; 
					 Debounce2->Enabled = true; SetDebounce2->Enabled = true;

					 InputMode3->Enabled = true; 
					 CounterReset3->Enabled = true; 
					 Debounce3->Enabled = true; SetDebounce3->Enabled = true;

					 InputMode4->Enabled = true; 
					 CounterReset4->Enabled = true; 
					 Debounce4->Enabled = true; SetDebounce4->Enabled = true;

					 InputMode5->Enabled = true; 
					 CounterReset5->Enabled = true; 
					 Debounce5->Enabled = true; SetDebounce5->Enabled = true;

					 PWMBar1->Enabled = true;
					 PWMBar2->Enabled = true;

					 O1->Enabled = true;
					 //OutputMode1->Enabled = true;

					 O2->Enabled = true;
					 //OutputMode2->Enabled = true;

					 O3->Enabled = true;
					 //OutputMode3->Enabled = true;

					 O4->Enabled = true;
					 //OutputMode4->Enabled = true;

					 O5->Enabled = true;
					 //OutputMode5->Enabled = true;

					 O6->Enabled = true;
					 //OutputMode6->Enabled = true;

					 O7->Enabled = true;
					 //OutputMode7->Enabled = true;

					 O8->Enabled = true;
					 //OutputMode8->Enabled = true;

					 updateAllConfig();
				 }
			 }
	private: System::Void buttonDisconnect_Click(System::Object^  sender, System::EventArgs^  e) {
				 if (cardHandle < 0)
					 return;

				 Open8055_Close(cardHandle);
				 cardHandle = -1;
				 updateAllConfig();
				 connectedMessage->Text = "Disconnected";

				 buttonDisconnect->Enabled = false;
				 buttonReset->Enabled = false;

				 ADC1->Text = "?"; ADCBar1->Value = 0;
				 ADC2->Text = "?"; ADCBar2->Value = 0;

				 I1->Checked = false; InputMode1->Enabled = false; 
				 Counter1->Text = "?"; CounterReset1->Enabled = false; 
				 Debounce1->Text = "?"; Debounce1->Enabled = false; SetDebounce1->Enabled = false;

				 I2->Checked = false; InputMode2->Enabled = false; 
				 Counter2->Text = "?"; CounterReset2->Enabled = false; 
				 Debounce2->Text = "?"; Debounce2->Enabled = false; SetDebounce2->Enabled = false;

				 I3->Checked = false; InputMode3->Enabled = false; 
				 Counter3->Text = "?"; CounterReset3->Enabled = false; 
				 Debounce3->Text = "?"; Debounce3->Enabled = false; SetDebounce3->Enabled = false;

				 I4->Checked = false; InputMode4->Enabled = false; 
				 Counter4->Text = "?"; CounterReset4->Enabled = false; 
				 Debounce4->Text = "?"; Debounce4->Enabled = false; SetDebounce4->Enabled = false;

				 I5->Checked = false; InputMode5->Enabled = false; 
				 Counter5->Text = "?"; CounterReset5->Enabled = false; 
				 Debounce5->Text = "?"; Debounce5->Enabled = false; SetDebounce5->Enabled = false;

				 PWM1->Text = "?"; PWMBar1->Value = 0; PWMBar1->Enabled = false;
				 PWM2->Text = "?"; PWMBar2->Value = 0; PWMBar2->Enabled = false;

				 O1->Checked = false; O1->Enabled = false;
				 OutputMode1->Enabled = false;

				 O2->Checked = false; O2->Enabled = false;
				 OutputMode2->Enabled = false;

				 O3->Checked = false; O3->Enabled = false;
				 OutputMode3->Enabled = false;

				 O4->Checked = false; O4->Enabled = false;
				 OutputMode4->Enabled = false;

				 O5->Checked = false; O5->Enabled = false;
				 OutputMode5->Enabled = false;

				 O6->Checked = false; O6->Enabled = false;
				 OutputMode6->Enabled = false;

				 O7->Checked = false; O7->Enabled = false;
				 OutputMode7->Enabled = false;

				 O8->Checked = false; O8->Enabled = false;
				 OutputMode8->Enabled = false;

			 }
	private: System::Void buttonReset_Click(System::Object^  sender, System::EventArgs^  e) {
				 if (cardHandle < 0)
					 return;

				 Open8055_Reset(cardHandle);
				 cardHandle = -1;
				 updateAllConfig();
				 connectedMessage->Text = "Disconnected";

				 buttonDisconnect->Enabled = false;
				 buttonReset->Enabled = false;

				 ADC1->Text = "?"; ADCBar1->Value = 0;
				 ADC2->Text = "?"; ADCBar2->Value = 0;

				 I1->Checked = false; InputMode1->Enabled = false; 
				 Counter1->Text = "?"; CounterReset1->Enabled = false; 
				 Debounce1->Text = "?"; Debounce1->Enabled = false; SetDebounce1->Enabled = false;

				 I2->Checked = false; InputMode2->Enabled = false; 
				 Counter2->Text = "?"; CounterReset2->Enabled = false; 
				 Debounce2->Text = "?"; Debounce2->Enabled = false; SetDebounce2->Enabled = false;

				 I3->Checked = false; InputMode3->Enabled = false; 
				 Counter3->Text = "?"; CounterReset3->Enabled = false; 
				 Debounce3->Text = "?"; Debounce3->Enabled = false; SetDebounce3->Enabled = false;

				 I4->Checked = false; InputMode4->Enabled = false; 
				 Counter4->Text = "?"; CounterReset4->Enabled = false; 
				 Debounce4->Text = "?"; Debounce4->Enabled = false; SetDebounce4->Enabled = false;

				 I5->Checked = false; InputMode5->Enabled = false; 
				 Counter5->Text = "?"; CounterReset5->Enabled = false; 
				 Debounce5->Text = "?"; Debounce5->Enabled = false; SetDebounce5->Enabled = false;

				 PWM1->Text = "?"; PWMBar1->Value = 0; PWMBar1->Enabled = false;
				 PWM2->Text = "?"; PWMBar2->Value = 0; PWMBar2->Enabled = false;

				 O1->Checked = false; O1->Enabled = false;
				 OutputMode1->Enabled = false;

				 O2->Checked = false; O2->Enabled = false;
				 OutputMode2->Enabled = false;

				 O3->Checked = false; O3->Enabled = false;
				 OutputMode3->Enabled = false;

				 O4->Checked = false; O4->Enabled = false;
				 OutputMode4->Enabled = false;

				 O5->Checked = false; O5->Enabled = false;
				 OutputMode5->Enabled = false;

				 O6->Checked = false; O6->Enabled = false;
				 OutputMode6->Enabled = false;

				 O7->Checked = false; O7->Enabled = false;
				 OutputMode7->Enabled = false;

				 O8->Checked = false; O8->Enabled = false;
				 OutputMode8->Enabled = false;

			 }
private: System::Void CounterReset1_Click(System::Object^  sender, System::EventArgs^  e) {
			 if (cardHandle >= 0)
				 Open8055_ResetCounter(cardHandle, 0);
		 }
private: System::Void CounterReset2_Click(System::Object^  sender, System::EventArgs^  e) {
			 if (cardHandle >= 0)
				 Open8055_ResetCounter(cardHandle, 1);
		 }
private: System::Void CounterReset3_Click(System::Object^  sender, System::EventArgs^  e) {
			 if (cardHandle >= 0)
				 Open8055_ResetCounter(cardHandle, 2);
		 }
private: System::Void CounterReset4_Click(System::Object^  sender, System::EventArgs^  e) {
			 if (cardHandle >= 0)
				 Open8055_ResetCounter(cardHandle, 3);
		 }
private: System::Void CounterReset5_Click(System::Object^  sender, System::EventArgs^  e) {
			 if (cardHandle >= 0)
				 Open8055_ResetCounter(cardHandle, 4);
		 }
private: System::Void SetDebounce1_Click(System::Object^  sender, System::EventArgs^  e) {
			 Double	value = 0.0;

			 if (cardHandle < 0)
				 return;

			 try {
				 value = Double::Parse(Debounce1->Text);
				 Open8055_SetDebounce(cardHandle, 0, value);
			 } catch (...) {}
			 Debounce1->Text = Open8055_GetDebounce(cardHandle, 0).ToString("F1");
		 }
private: System::Void SetDebounce2_Click(System::Object^  sender, System::EventArgs^  e) {
			 Double	value = 0.0;

			 if (cardHandle < 0)
				 return;

			 try {
				 value = Double::Parse(Debounce2->Text);
				 Open8055_SetDebounce(cardHandle, 1, value);
			 } catch (...) {}
			 Debounce2->Text = Open8055_GetDebounce(cardHandle, 1).ToString("F1");
		 }
private: System::Void SetDebounce3_Click(System::Object^  sender, System::EventArgs^  e) {
			 Double	value = 0.0;

			 if (cardHandle < 0)
				 return;

			 try {
				 value = Double::Parse(Debounce3->Text);
				 Open8055_SetDebounce(cardHandle, 2, value);
			 } catch (...) {}
			 Debounce3->Text = Open8055_GetDebounce(cardHandle, 2).ToString("F1");
		 }
private: System::Void SetDebounce4_Click(System::Object^  sender, System::EventArgs^  e) {
			 Double	value = 0.0;

			 if (cardHandle < 0)
				 return;

			 try {
				 value = Double::Parse(Debounce4->Text);
				 Open8055_SetDebounce(cardHandle, 3, value);
			 } catch (...) {}
			 Debounce4->Text = Open8055_GetDebounce(cardHandle, 3).ToString("F1");
		 }
private: System::Void SetDebounce5_Click(System::Object^  sender, System::EventArgs^  e) {
			 Double	value = 0.0;

			 if (cardHandle < 0)
				 return;

			 try {
				 value = Double::Parse(Debounce5->Text);
				 Open8055_SetDebounce(cardHandle, 4, value);
			 } catch (...) {}
			 Debounce5->Text = Open8055_GetDebounce(cardHandle, 4).ToString("F1");
		 }

private: System::Void InputMode1_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 Open8055_SetModeInput(cardHandle, 0, modeInputFromIndex(InputMode1->SelectedIndex));
			 updateAllConfig();
		 }
private: System::Void InputMode2_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 Open8055_SetModeInput(cardHandle, 1, modeInputFromIndex(InputMode2->SelectedIndex));
			 updateAllConfig();
		 }
private: System::Void InputMode3_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 Open8055_SetModeInput(cardHandle, 2, modeInputFromIndex(InputMode3->SelectedIndex));
			 updateAllConfig();
		 }
private: System::Void InputMode4_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 Open8055_SetModeInput(cardHandle, 3, modeInputFromIndex(InputMode4->SelectedIndex));
			 updateAllConfig();
		 }
private: System::Void InputMode5_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 Open8055_SetModeInput(cardHandle, 4, modeInputFromIndex(InputMode5->SelectedIndex));
			 updateAllConfig();
		 }
private: System::Void PWMBar1_Scroll(System::Object^  sender, System::Windows::Forms::ScrollEventArgs^  e) {
			 if (cardHandle >= 0)
			 {
				 Open8055_SetPWM(cardHandle, 0, PWMBar1->Value);
				 PWM1->Text = Open8055_GetPWM(cardHandle, 0).ToString("d");
			 }
		 }
private: System::Void PWMBar2_Scroll(System::Object^  sender, System::Windows::Forms::ScrollEventArgs^  e) {
			 if (cardHandle >= 0)
			 {
				 Open8055_SetPWM(cardHandle, 1, PWMBar2->Value);
				 PWM2->Text = Open8055_GetPWM(cardHandle, 1).ToString("d");
			 }
		 }
private: System::Void O1_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (cardHandle >= 0)
			 {
				 Open8055_SetOutput(cardHandle, 0, O1->Checked);
				 O1->Checked = Open8055_GetOutput(cardHandle, 0) != 0;
			 }
		 }
private: System::Void O2_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (cardHandle >= 0)
			 {
				 Open8055_SetOutput(cardHandle, 1, O2->Checked);
				 O2->Checked = Open8055_GetOutput(cardHandle, 1) != 0;
			 }
		 }
private: System::Void O3_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (cardHandle >= 0)
			 {
				 Open8055_SetOutput(cardHandle, 2, O3->Checked);
				 O3->Checked = Open8055_GetOutput(cardHandle, 2) != 0;
			 }
		 }
private: System::Void O4_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (cardHandle >= 0)
			 {
				 Open8055_SetOutput(cardHandle, 3, O4->Checked);
				 O4->Checked = Open8055_GetOutput(cardHandle, 3) != 0;
			 }
		 }
private: System::Void O5_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (cardHandle >= 0)
			 {
				 Open8055_SetOutput(cardHandle, 4, O5->Checked);
				 O5->Checked = Open8055_GetOutput(cardHandle, 4) != 0;
			 }
		 }
private: System::Void O6_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (cardHandle >= 0)
			 {
				 Open8055_SetOutput(cardHandle, 5, O6->Checked);
				 O6->Checked = Open8055_GetOutput(cardHandle, 5) != 0;
			 }
		 }
private: System::Void O7_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (cardHandle >= 0)
			 {
				 Open8055_SetOutput(cardHandle, 6, O7->Checked);
				 O7->Checked = Open8055_GetOutput(cardHandle, 6) != 0;
			 }
		 }
private: System::Void O8_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (cardHandle >= 0)
			 {
				 Open8055_SetOutput(cardHandle, 7, O8->Checked);
				 O8->Checked = Open8055_GetOutput(cardHandle, 7) != 0;
			 }
		 }
private: System::Void backgroundWorker1_DoWork(System::Object^  sender, System::ComponentModel::DoWorkEventArgs^  e) {
			 while (cardHandle >= 0)
			 {
				 if (Open8055_WaitForReport(cardHandle) < 0)
				 {
					 backgroundWorker1->ReportProgress(-1);
					 Sleep(1000);
					 continue;
				 }

				 backgroundWorker1->ReportProgress(0);
			 }
		 }
private: System::Void backgroundWorker1_ProgressChanged(System::Object^  sender, ProgressChangedEventArgs^  e) {
			 int	value;
		
			 if (cardHandle < 0)
				 return;

			 if (e->ProgressPercentage < 0)
			 {
				 connectedMessage->Text = "BGworker: " +
						 Marshal::PtrToStringAnsi((System::IntPtr)Open8055_LastError(cardHandle));
				 return;
			 }

			 value = Open8055_GetInputAll(cardHandle);

			 if (value < 0)
			 {
				 connectedMessage->Text = "Connection lost";
				 return;
			 }

			 I1->Checked = (value & 0x01) != 0;
			 I2->Checked = (value & 0x02) != 0;
			 I3->Checked = (value & 0x04) != 0;
			 I4->Checked = (value & 0x08) != 0;
			 I5->Checked = (value & 0x10) != 0;

			 ADCBar1->Value = Open8055_GetADC(cardHandle, 0);
			 ADC1->Text = ADCBar1->Value.ToString();
			 ADCBar2->Value = Open8055_GetADC(cardHandle, 1);
			 ADC2->Text = ADCBar2->Value.ToString();

			 Counter1->Text = Open8055_GetCounter(cardHandle, 0).ToString();
			 Counter2->Text = Open8055_GetCounter(cardHandle, 1).ToString();
			 Counter3->Text = Open8055_GetCounter(cardHandle, 2).ToString();
			 Counter4->Text = Open8055_GetCounter(cardHandle, 3).ToString();
			 Counter5->Text = Open8055_GetCounter(cardHandle, 4).ToString();
		 }
};
}

