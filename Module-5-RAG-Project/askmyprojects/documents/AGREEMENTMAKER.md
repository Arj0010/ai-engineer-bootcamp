# Agreement Maker 📜

> AI-Powered Legal Assistant Suite for Contract Generation and Legal Consultation

Agreement Maker is a full-stack web application that combines AI-powered contract generation with an interactive legal chat assistant. Built with Flask and Groq AI (Llama 3.3), it provides professional-grade legal document drafting and consultation tools.

![Python](https://img.shields.io/badge/python-3.8+-blue.svg)
![Flask](https://img.shields.io/badge/flask-2.3.3-green.svg)
![Groq](https://img.shields.io/badge/AI-Groq%20Llama%203.3-orange.svg)
![License](https://img.shields.io/badge/license-MIT-blue.svg)

---

## ✨ Features

### 🤖 AI-Powered Contract Generation
- **24 Contract Types**: Employment, NDA, Lease, Partnership, and 20 more
- **Intelligent Fact Extraction**: Automatically identifies parties, dates, amounts from descriptions
- **18-Point Legal Structure**: Industry-standard contract framework
- **AI Refinement**: Optional Groq AI enhancement for professional polish

### 💬 Legal Chat Assistant
- **Real-time Q&A**: Ask legal questions and get informed responses
- **Modern Chat UI**: Bubble interface with typing indicators
- **Context-Aware**: Understands legal terminology and concepts
- **Markdown Support**: Formatted responses with lists and emphasis

### 📄 Document Export
- **Multiple Formats**: Download contracts as TXT, DOCX, or PDF
- **Professional Formatting**: Proper margins, fonts, and styling
- **Instant Download**: One-click export from browser

### 🎨 User Interface
- **Modern Design**: Gradient backgrounds, smooth animations
- **Responsive Layout**: Works on desktop and mobile
- **Loading Indicators**: Clear feedback during AI processing
- **Form Validation**: Input checking with character counters

---

## 🚀 Quick Start

### Prerequisites
- Python 3.8 or higher
- Groq API Key ([Get one free](https://console.groq.com/keys))

### Installation

```bash
# 1. Clone the repository
git clone <repository-url>
cd Agreement_Maker/contract_drafter

# 2. Create virtual environment
python -m venv .venv

# 3. Activate virtual environment
# Windows:
.venv\Scripts\activate
# macOS/Linux:
source .venv/bin/activate

# 4. Install dependencies
pip install -r requirements.txt

# 5. Set up environment variables
# Create a .env file with:
echo "GROQ_API_KEY=your_groq_api_key_here" > .env

# 6. Run the application


```

### Access the Application
- **Agreement Maker**: http://localhost:5000/
- **Legal Chat**: http://localhost:5000/legal

---

## 📁 Project Structure

```
Agreement_Maker/
├── contract_drafter/           # Main application directory
│   ├── app.py                  # Flask backend server
│   ├── contract_generator.py   # Contract skeleton builder
│   ├── groq_interface.py       # Groq AI integration
│   ├── contract_web_scraper.py # Clause scraping utility
│   ├── requirements.txt        # Python dependencies
│   ├── .env                    # Environment variables (create this)
│   │
│   ├── templates/              # HTML templates
│   │   ├── agreement_maker.html
│   │   └── legal_chat.html
│   │
│   ├── static/                 # CSS and static assets
│   │   ├── legal_style.css
│   │   ├── agreement_maker.css
│   │   └── style.css
│   │
│   ├── utils/                  # Utility modules
│   │   ├── fact_extractor.py  # Extract facts from text
│   │   └── prompt_builder.py  # Build AI prompts
│   │
│   ├── docs/                   # Documentation
│   │   ├── SETUP_AND_TEST.md  # Setup & testing guide
│   │   ├── GROQ_FIX_FINAL.md  # Groq API configuration
│   │   └── IMPLEMENTATION_SUMMARY.md
│   │
│   └── tests/                  # Test scripts
│       ├── test_groq_api.py   # API connectivity test
│       └── test.py
│
├── README.md                   # This file
├── .gitignore
└── Agreement_Maker.code-workspace
```

---

## 🎯 Usage

### Generate a Contract

1. **Open Agreement Maker** at http://localhost:5000/
2. **Select Contract Type**: Click a tile or use the dropdown
3. **Add Description**: Enter details like parties, amounts, dates
4. **Enable AI Refinement**: Check the box for AI enhancement (recommended)
5. **Generate**: Click "Generate Contract"
6. **Download**: Export as TXT, DOCX, or PDF

**Example Description:**
```
Employment Agreement between TechCorp Inc. and John Doe
for Senior Software Engineer position.
- Start Date: January 1, 2025
- Salary: $120,000/year
- Location: San Francisco, CA
- Benefits: Health insurance, 401k, PTO
- Probation: 90 days
```

### Use Legal Chat

1. **Open Legal Chat** at http://localhost:5000/legal
2. **Ask Questions**: Type legal questions in natural language
3. **Get Answers**: Receive informed responses from AI
4. **Continue Conversation**: Chat history is maintained

**Example Questions:**
- "What is a non-disclosure agreement?"
- "Explain the difference between a lease and a rental agreement"
- "What are key clauses in an employment contract?"

---

## 🛠️ Configuration

### Environment Variables

Create a `.env` file in `contract_drafter/`:

```env
# Required: Groq API Key
GROQ_API_KEY=gsk_your_api_key_here

# Optional: Model Selection
GROQ_MODEL=llama-3.3  # Options: llama-3.3, llama-3.1, mixtral, gemma2

# Optional: Flask Configuration
FLASK_RUN_PORT=5000
FLASK_DEBUG=1
```

### Available Models

| Model | Full Name | Speed | Quality | Best For |
|-------|-----------|-------|---------|----------|
| **llama-3.3** (default) | llama-3.3-70b-versatile | Medium | Excellent | Contract generation, legal advice |
| llama-3.1 | llama-3.1-70b-versatile | Medium | Very Good | Alternative to 3.3 |
| llama-3.1-8b | llama-3.1-8b-instant | Fast | Good | Quick responses |
| mixtral | mixtral-8x7b-32768 | Medium | Very Good | Alternative option |
| gemma2 | gemma2-9b-it | Fast | Good | Lightweight tasks |

---

## 🧪 Testing

### Test Groq API Connection
```bash
cd contract_drafter
python tests/test_groq_api.py
```

Expected output:
```
✅ SUCCESS with llama-3.3-70b-versatile!
   Response: Hello from Groq!
```

### Manual Testing Checklist

#### Agreement Maker
- [ ] Page loads without errors
- [ ] Contract type tiles are clickable
- [ ] Description auto-fills on type selection
- [ ] Character counter updates
- [ ] Generate button shows loading overlay
- [ ] Contract appears in output panel
- [ ] Copy button works
- [ ] TXT download works
- [ ] DOCX download works
- [ ] PDF download works

#### Legal Chat
- [ ] Page loads with welcome message
- [ ] Input textarea accepts text
- [ ] Character counter updates
- [ ] Send button is enabled with text
- [ ] Enter key sends message
- [ ] User messages appear on right (blue)
- [ ] AI responses appear on left (grey)
- [ ] Typing indicator shows while processing
- [ ] Chat scrolls to bottom automatically

---

## 📚 Documentation

Detailed documentation is available in the `contract_drafter/docs/` directory:

- **[SETUP_AND_TEST.md](contract_drafter/docs/SETUP_AND_TEST.md)** - Complete setup and testing guide
- **[GROQ_FIX_FINAL.md](contract_drafter/docs/GROQ_FIX_FINAL.md)** - Groq API configuration and troubleshooting
- **[IMPLEMENTATION_SUMMARY.md](contract_drafter/docs/IMPLEMENTATION_SUMMARY.md)** - Technical implementation details

---

## 🔧 Troubleshooting

### Common Issues

**Issue: ModuleNotFoundError**
```bash
# Solution: Activate virtual environment and install dependencies
.venv\Scripts\activate
pip install -r requirements.txt
```

**Issue: Missing GROQ_API_KEY**
```bash
# Solution: Create .env file with your API key
echo "GROQ_API_KEY=your_key" > .env
```

**Issue: Groq API 400 Error**
```bash
# Solution: Test API connectivity
python tests/test_groq_api.py
# If models are outdated, update groq_interface.py
```

**Issue: Port 5000 already in use**
```python
# Solution: Change port in app.py (line ~335)
app.run(host="0.0.0.0", port=5001, debug=True)
```

**Issue: Templates not found**
```bash
# Solution: Run from correct directory
cd contract_drafter
python app.py
```

### Get Help

- Check [SETUP_AND_TEST.md](contract_drafter/docs/SETUP_AND_TEST.md) for detailed troubleshooting
- Review [Groq API Status](https://status.groq.com/)
- Verify API key at [Groq Console](https://console.groq.com/keys)

---

## 🏗️ Technology Stack

### Backend
- **Flask 2.3.3** - Web framework
- **Python 3.8+** - Programming language
- **Groq API** - AI model inference
- **python-docx** - DOCX generation
- **ReportLab** - PDF generation

### Frontend
- **HTML5/CSS3** - Structure and styling
- **JavaScript (ES6+)** - Interactivity
- **jQuery** - DOM manipulation
- **Marked.js** - Markdown rendering
- **Font Awesome** - Icons

### AI Models
- **Llama 3.3 70B** - Primary model (via Groq)
- **Llama 3.1 70B** - Alternative
- **Mixtral 8x7B** - Fallback option

---

## 🤝 Contributing

Contributions are welcome! Here's how you can help:

1. **Report Bugs**: Open an issue with details
2. **Suggest Features**: Describe your idea
3. **Submit Pull Requests**: Fork, create branch, commit, push, PR
4. **Improve Documentation**: Fix typos, add examples

### Development Setup
```bash
# Clone and setup
git clone <repo-url>
cd Agreement_Maker/contract_drafter

# Create virtual environment
python -m venv .venv
.venv\Scripts\activate

# Install dev dependencies
pip install -r requirements.txt

# Run tests
python tests/test_groq_api.py

# Start development server
python app.py
```

---

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

---

## 🙏 Acknowledgments

- **Groq** - For providing fast AI inference API
- **Meta AI** - For Llama models
- **Flask** - For the excellent web framework
- **ReportLab** - For PDF generation capabilities

---

## 📞 Support

- **Documentation**: [docs/](contract_drafter/docs/)
- **Issues**: Open a GitHub issue
- **API Support**: [Groq Console](https://console.groq.com/)

---

## 🗺️ Roadmap

### Planned Features
- [ ] User authentication and accounts
- [ ] Contract history and storage
- [ ] Multi-language support
- [ ] Contract comparison tool
- [ ] Email delivery integration
- [ ] Template library expansion
- [ ] Collaborative editing
- [ ] Version control for contracts

### In Progress
- [x] Core contract generation
- [x] Legal chat assistant
- [x] Document export (TXT/DOCX/PDF)
- [x] Groq AI integration

---

## 🎉 Success Stories

> "Agreement Maker helped me draft a comprehensive NDA in under 5 minutes. The AI refinement feature added professional clauses I hadn't thought of!" - *Beta Tester*

> "The legal chat assistant is incredibly helpful for understanding contract terms before signing." - *Small Business Owner*

---

**Made with ❤️ using Flask, Python, and Groq AI**

*For the latest updates and version history, see the [docs/](contract_drafter/docs/) directory.*
