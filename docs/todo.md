# 封板后到predict_t时刻之间涨停价订单加权平均撤单时间
    def factor031(self):
        total_weighted_time = 0
        total_volume = 0
        cancel_order_ids = self.order[(self.order['time'] >= self.lock_time) &
                        (self.order['time'] <= self.predict_t) &
                        (self.order['orderPrice'] == self.limit_up) &
                        (self.order['orderKind'] == 'D')]['orderOriNo'].to_list()
        for cancel_order_id in cancel_order_ids:
            temp_order = self.order[self.order['orderOriNo'] == cancel_order_id]
            duration = time_sub(temp_order['time'].iloc[-1], temp_order['time'].iloc[0])
            volume = temp_order['orderVolume'].iloc[0]
            total_weighted_time += duration * volume
            total_volume += volume
        result = total_weighted_time / total_volume if total_volume > 0 else 0
        return result
    
    # 封板时刻指数增长模型的alpha
    def factor032(self):
        s = self.cross_section[(self.cross_section['time'] == self.lock_time) &
                                (self.cross_section['委买总额'] > 0)]['委买总额']
        if s.empty:
            return np.nan
        Q0 = s.iloc[0]
        Qt = s.iloc[-1]
        t = len(s)
        alpha = np.log(Qt / Q0) / t
        return alpha
    
    # predict_t时刻指数增长模型的alpha
    def factor033(self):
        s = self.cross_section[(self.cross_section['time'] == self.predict_t) &
                                (self.cross_section['委买总额'] > 0)]['委买总额']
        if s.empty:
            return np.nan
        Q0 = s.iloc[0]
        Qt = s.iloc[-1]
        t = len(s)
        alpha = np.log(Qt / Q0) / t
        return alpha
    
    # 封板后到predict_t时刻之间截面数据的count
    def factor034(self):
        temp_cross_section = self.cross_section[(self.cross_section['time'] >= self.lock_time) &
                                                (self.cross_section['time'] <= self.predict_t)]
        return len(temp_cross_section)
    
    # 封板后到predict_t时刻之间截面数据的更新频率
    def factor035(self):
        temp_cross_section = self.cross_section[(self.cross_section['time'] >= self.lock_time) &
                                                (self.cross_section['time'] <= self.predict_t)]
        return len(temp_cross_section) / time_sub(self.predict_t, self.lock_time) if time_sub(self.predict_t, self.lock_time) != 0 else len(self.cross_section)
    
    # 封板后到predict_t时刻的截面数据做差分, 计算predict_t时刻的VaR95
    def factor036(self):
        temp_cross_section = self.cross_section[(self.cross_section['time'] >= self.lock_time) &
                                                (self.cross_section['time'] <= self.predict_t)]
        s = (temp_cross_section['委买总额'] / temp_cross_section['委买总额'].shift(1) - 1).fillna(0).copy()
        var_95 = s.quantile(0.05)
        return var_95
    
    # 封板后到predict_t时刻的截面数据做差分, 计算predict_t时刻的cVaR95
    def factor037(self):
        temp_cross_section = self.cross_section[(self.cross_section['time'] >= self.lock_time) &
                                                (self.cross_section['time'] <= self.predict_t)]
        s = (temp_cross_section['委买总额'] / temp_cross_section['委买总额'].shift(1) - 1).fillna(0).copy()
        var_95 = s.quantile(0.05)
        return s[s < var_95].mean()
    
    # 封板后到predict_t时刻之间的上升梯度
    def factor038(self):
        temp_cross_section = self.cross_section[(self.cross_section['time'] >= self.lock_time) &
                                                (self.cross_section['time'] <= self.predict_t)]
        max_amount_time = temp_cross_section.loc[temp_cross_section['委买总额'].idxmax(), 'time']
        max_amount = temp_cross_section.loc[temp_cross_section['委买总额'].idxmax(), '委买总额']
        return max_amount / time_sub(max_amount_time, self.lock_time) if time_sub(max_amount_time, self.lock_time) != 0 else max_amount
    
    # 封板后到predict_t时刻之间的下降梯度
    def factor039(self):
        temp_cross_section = self.cross_section[(self.cross_section['time'] >= self.lock_time) &
                                                (self.cross_section['time'] <= self.predict_t)]
        max_amount_time = temp_cross_section.loc[temp_cross_section['委买总额'].idxmax(), 'time']
        max_amount = temp_cross_section.loc[temp_cross_section['委买总额'].idxmax(), '委买总额']
        last_amount = temp_cross_section['委买总额'].iloc[-1]
        return (last_amount - max_amount) / time_sub(self.predict_t, max_amount_time) if time_sub(self.predict_t, max_amount_time) != 0 else (last_amount - max_amount)
    
    # 封板后到predict_t时刻之间委买总额的偏度
    def factor040(self):
        temp_cross_section = self.cross_section[(self.cross_section['time'] >= self.lock_time) &
                                                (self.cross_section['time'] <= self.predict_t)]
        return temp_cross_section['委买总额'].skew()
    
    # 封板后70ms内涨停价新增挂单量除以封板后70ms内涨停价新增挂单数
    def factor041(self):
        if time_sub(self.predict_t, self.lock_time) < 70:
            return np.nan
        temp_order = self.order[(self.order['time'] >= self.lock_time) &
                                (self.order['time'] <= time_add(self.lock_time, 70)) &
                                (self.order['orderKind'] == 'A') &
                                (self.order['orderPrice'] == self.limit_up) &
                                (self.order['functionCode'] == 'B')].copy()
        return temp_order['orderVolume'].sum() / len(temp_order)
    