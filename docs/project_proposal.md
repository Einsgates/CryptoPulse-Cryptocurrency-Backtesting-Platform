# Project Proposal

## Project Overview

Our goal is to produce a backtesting software for cryptocurrency with the following features:

- Supporting All Exchanges with Their Own Trade Rules
- Latency Analysis
- Configurable Fee Structure

## Technologies and Tools

- **Programming Languages**:  
  * C++

- **Libraries and Frameworks**: 
  * RapidJson 
  * Google Tests

- **Cryptocurrency Exchanges**: 
  * Binance
  * Bybit
  * Coinbase
  * Okx

## CI/CD (Continuous Integration/Continuous Deployment)

- **Automated Testing**: 
  * Google Test
  * GitLab

## Project Timeline
- **02/26/2024**: 
  - *Goal:* 
    - Completion of the orderbook prototype.
    - Completion of unit test on orders and exchanges.

- **04/02/2024**: 
  - *Goal:*
    - Create an order matching engine (OME) prototype having self-matching prevention.
    - Completion of unit test on order book.

- **04/09/2024**: 
  - *Goals:*
    - Combine everything to have the back-end prototype.
    - Completion of unit test on OME.

- **04/16/2024**: 
  - *Goals*: 
    - Completion of back-end side of backtester.

- **04/23/2024**: 
  - *Goals*: 
    - Completion of overall unit test.
    - Start working on latency analysis.

- **04/30/2024**: 
  - *Goals*: 
    - Start working on reach goals on implementing API connection to major exchanges.
    - Start working on the final report

- **05/07/2024**: 
  - *Goals*: 
    - Final review of the project and report.



## Project Goals

1. **Bare Minimum Target**: 
  * Create a backtesting software that accepts any crypto exchange and security. 
  * Enable more user freedom through having variables configurable.
2. **Expected Completion Goals**: 
  * Provide latency analysis (Latency vs P&L)
  * Simulate more realistic fills/slippages for backtesting
3. **Reach Goals**: 
  * Not only provide backtesting feature, but also allow users to directly connect to an exchange through APIs, so that users could paper trade or live trade. 

  ## Deliverables

- **Code Repository:** Located on GitLab documenting development process.

- **Report:**  Located on Gitlab containing detailed description on the architecture, documentations, key features, and usages.  

## Team Members

- [Team Member 1]: Min Hyuk "Michael" Bhang
  - Graduation Semester: Spring 2024
  - Future Plans: Master's in Financial Engineering (MSFE)

- [Team Member 2]: Jerry (Jiayuan) Hong
  - Graduation Semester: Spring 2025
  - Future Plans: Software Engineer in a trading company

- [Team Member 3]: Ze-Xuan Liu
  - Graduation Semester: Summer 2027
  - Future Plans: Buy-side Quant

