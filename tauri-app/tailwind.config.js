/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        paper: {
          DEFAULT: '#fdfbf7',
          dark: '#f5f2eb',
        },
        sage: {
          50: '#f4f7f4',
          100: '#e3ebe3',
          200: '#c5d8c5',
          300: '#a3c2a3',
          400: '#84a98c', // Primary Sage
          500: '#6b9074',
          600: '#55765d',
          700: '#415a47',
          800: '#2f3f33',
        },
        clay: {
          500: '#e07a5f', // Warm terracotta accent
          600: '#c46248',
        },
        ink: {
          400: '#9ca3af',
          600: '#4b5563', // Body text
          800: '#1f2937', // Headings
        },
      },
      fontFamily: {
        sans: ['"Nunito"', '"Quicksand"', 'system-ui', 'sans-serif'],
      },
      borderRadius: {
        'xl': '1rem',
        '2xl': '1.5rem',
        '3xl': '2rem',
      }
    },
    animation: {
      'pulse-slow': 'pulse 3s cubic-bezier(0.4, 0, 0.6, 1) infinite',
    }
  },
  plugins: [],
}
