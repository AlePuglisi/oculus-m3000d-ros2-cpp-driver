# Configuration file for the Sphinx documentation builder.

project = "Oculus M3000d Sonar ROS2 Driver"
author = "Alessandro Puglisi"
release = "1.0"

extensions = [
    "myst_parser",
    "sphinx_design",      # dropdowns, tabs, cards
    "sphinx_copybutton",  # copy button on code blocks
]

myst_enable_extensions = ["colon_fence", "deflist", "html_image"]
myst_heading_anchors = 3   # allows links like installation.md#some-heading

source_suffix = {".rst": "restructuredtext", ".md": "markdown"}
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

html_theme = "sphinx_rtd_theme"
html_static_path = []
html_theme_options = {
    "collapse_navigation": False,
    "navigation_depth": 3,
}