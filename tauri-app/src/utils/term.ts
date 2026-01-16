
/**
 * Calculates the current school term based on the current date.
 * Logic:
 * - September (Month 8) to January (Month 0): Term 1
 * - February (Month 1) to August (Month 7): Term 2
 * 
 * Note: January belongs to the Fall semester of the previous calendar year.
 */
export const getCurrentTerm = (): string => {
    const now = new Date();
    const month = now.getMonth(); // 0-11
    const year = now.getFullYear();

    let startYear, endYear, term;

    // September to December
    if (month >= 8) {
        startYear = year;
        endYear = year + 1;
        term = 1;
    }
    // January (belongs to Term 1 of previous start year)
    else if (month === 0) {
        startYear = year - 1;
        endYear = year;
        term = 1;
    }
    // February to August
    else {
        startYear = year - 1;
        endYear = year;
        term = 2;
    }

    return `${startYear}-${endYear}-${term}`;
};
