/* Copyright (C) 2013 Interactive Brokers LLC. All rights reserved.  This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace IBApi
{
    /**
     * @class CommissionReport
     * @brief class representing the commissions generated by an execution.
     * @sa Execution
     */
    [ComVisible(true)]
    public class CommissionReport : TWSLib.ICommissionReport
    {
        private string execId;
        private double commission;
        private string currency;
        private double realizedPNL;
        private double yield;
        private int yieldRedemptionDate;

        /**
        * @brief the execution's id this commission belongs to.
        */
        public string ExecId
        {
            get { return execId; }
            set { execId = value; }
        }

        /**
         * @brief the commissions cost.
         */
        public double Commission
        {
            get { return commission; }
            set { commission = value; }
        }

        /**
        * @brief the reporting currency.
        */
        public string Currency
        {
            get { return currency; }
            set { currency = value; }
        }

        /**
        * @brief the realised profit and loss
        */
        public double RealizedPNL
        {
            get { return realizedPNL; }
            set { realizedPNL = value; }
        }

        /**
         * @brief The income return.
         */
        public double Yield
        {
            get { return yield; }
            set { yield = value; }
        }

        /**
         * @brief date expressed in yyyymmdd format.
         */
        public int YieldRedemptionDate
        {
            get { return yieldRedemptionDate; }
            set { yieldRedemptionDate = value; }
        }

        public CommissionReport()
        {
            commission = 0;
            realizedPNL = 0;
            yield = 0;
            yieldRedemptionDate = 0;
        }

        public override bool Equals(Object p_other)
        {
            bool l_bRetVal = false;

            if (p_other == null)
            {
                l_bRetVal = false;
            }
            else if (this == p_other)
            {
                l_bRetVal = true;
            }
            else
            {
                CommissionReport l_theOther = (CommissionReport)p_other;
                l_bRetVal = ExecId.Equals(l_theOther.ExecId);
            }
            return l_bRetVal;
        }

        string TWSLib.ICommissionReport.execId
        {
            get { return execId; }
        }

        double TWSLib.ICommissionReport.commission
        {
            get { return commission; }
        }

        string TWSLib.ICommissionReport.currency
        {
            get { return currency; }
        }

        double TWSLib.ICommissionReport.realizedPNL
        {
            get { return realizedPNL; }
        }

        double TWSLib.ICommissionReport.yield
        {
            get { return yield; }
        }

        int TWSLib.ICommissionReport.yieldRedemptionDate
        {
            get { return yieldRedemptionDate; }
        }
    }
}
